#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include "hal/msg/hal_depthsensor.hpp" 
#include <std_msgs/msg/string.hpp>   // 【新增】用于发布连接状态
#include <cmath> 
#include <algorithm> // 包含 std::max
#include <atomic>    // 包含 std::atomic

// 引入 Linux 底层网络与 SocketCAN 系统头文件
#include <cstring>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/select.h>
#include <thread>
#include <mutex>
#include <chrono>

using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;
using namespace std::chrono_literals;

/**
 * @brief 原生 Linux SocketCAN 初始化函数 
 */
int setup_can_socket(const std::string& interface_name) {
    int s;
    struct sockaddr_can addr;
    struct ifreq ifr;

    // 创建 RAW CAN socket
    if ((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0) {
        return -1;
    }

    // 获取接口索引
    std::strncpy(ifr.ifr_name, interface_name.c_str(), IFNAMSIZ - 1);
    ifr.ifr_name[IFNAMSIZ - 1] = '\0';
    if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
        close(s);
        return -1;
    }

    // 绑定 Socket 到指定 CAN 接口
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(s);
        return -1;
    }

    // 设置 Socket 为非阻塞模式，防止死锁
    int flags = fcntl(s, F_GETFL, 0);
    fcntl(s, F_SETFL, flags | O_NONBLOCK);

    return s;
}

class HalDepthSensorNode : public rclcpp_lifecycle::LifecycleNode
{
public:
    HalDepthSensorNode(const std::string & node_name)
    : rclcpp_lifecycle::LifecycleNode(node_name)
    {
        this->declare_parameter<std::string>("can_interface", "can0");
        
        // 初始化缓存消息，防止初始时刻发送未定义的随机内存值
        cached_msg_.depth_1 = 0.0f;
        cached_msg_.depth_2 = 0.0f;
        cached_msg_.temp_1 = 0.0f;
        cached_msg_.temp_2 = 0.0f;
        cached_msg_.depth_avg = 0.0f;
        cached_msg_.timestamp = 0;
    }

    CallbackReturn on_configure(const rclcpp_lifecycle::State &) override {
        RCLCPP_INFO(get_logger(), "配置中... 初始化水深传感器发布者 (固定 50Hz)。");
        depth_pub_ = this->create_publisher<hal::msg::HalDepthsensor>("/hal/depthsenor", 10);
        
        // 【新增】初始化状态发布者
        status_pub_ = this->create_publisher<std_msgs::msg::String>("hal_depthsensor_status", 10);
        
        // 创建 50Hz (20ms) 定时器，专门负责数据发布
        publish_timer_ = this->create_wall_timer(
            20ms, std::bind(&HalDepthSensorNode::publish_timer_callback, this));

        // 创建 1Hz 看门狗定时器，专门负责监控传感器掉线重连
        watchdog_timer_ = this->create_wall_timer(
            1s, std::bind(&HalDepthSensorNode::watchdog_callback, this));

        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_activate(const rclcpp_lifecycle::State & state) override {
        std::string can_iface = this->get_parameter("can_interface").as_string();
        can_socket_ = setup_can_socket(can_iface);
        
        if (can_socket_ < 0) {
            RCLCPP_ERROR(this->get_logger(), "无法打开 CAN 接口: %s", can_iface.c_str());
            return CallbackReturn::FAILURE;
        }

        // 发送 CANOpen NMT 启动指令，唤醒总线上所有水深传感器
        send_nmt_wakeup();

        depth_pub_->on_activate();
        status_pub_->on_activate(); // 【新增】激活状态发布者
        
        is_running_ = true;
        is_connected_ = false; // 初始设定为断开，等待接收到第一帧数据后触发连接发布
        can_thread_ = std::thread(&HalDepthSensorNode::can_thread_function, this);
        return LifecycleNode::on_activate(state);
    }

    CallbackReturn on_deactivate(const rclcpp_lifecycle::State & state) override {
        depth_pub_->on_deactivate();
        status_pub_->on_deactivate(); // 【新增】去激活状态发布者
        
        is_running_ = false;
        if (can_thread_.joinable()) can_thread_.join();
        
        if (can_socket_ >= 0) {
            close(can_socket_);
            can_socket_ = -1;
        }
        return LifecycleNode::on_deactivate(state);
    }

    CallbackReturn on_cleanup(const rclcpp_lifecycle::State &) override {
        depth_pub_.reset();
        status_pub_.reset(); // 【新增】清理状态发布者
        publish_timer_.reset();
        watchdog_timer_.reset(); 
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_shutdown(const rclcpp_lifecycle::State &) override {
        is_running_ = false;
        if (can_thread_.joinable()) can_thread_.join();
        
        if (can_socket_ >= 0) {
            close(can_socket_);
            can_socket_ = -1;
        }
        return CallbackReturn::SUCCESS;
    }

private:
    std::shared_ptr<rclcpp_lifecycle::LifecyclePublisher<hal::msg::HalDepthsensor>> depth_pub_;
    std::shared_ptr<rclcpp_lifecycle::LifecyclePublisher<std_msgs::msg::String>> status_pub_; // 【新增】状态发布者指针
    
    rclcpp::TimerBase::SharedPtr publish_timer_;
    rclcpp::TimerBase::SharedPtr watchdog_timer_; 

    hal::msg::HalDepthsensor cached_msg_;
    std::mutex msg_mutex_;

    int can_socket_ = -1;
    std::thread can_thread_;
    std::atomic<bool> is_running_{false};
    
    // 【新增】连接状态标志位，使用 atomic 保证多线程安全
    std::atomic<bool> is_connected_{false}; 

    // 记录两个传感器是否更新过数据，用于计算平均值
    bool has_data_1_ = false;
    bool has_data_2_ = false;

    // 【新增】发布连接状态的工具函数
    void publish_status(const std::string& state_str) {
        if (status_pub_ && status_pub_->is_activated()) {
            std_msgs::msg::String msg;
            msg.data = state_str;
            status_pub_->publish(msg);
        }
    }

    // 封装好的唤醒指令函数
    void send_nmt_wakeup() {
        if (can_socket_ < 0) return;
        struct can_frame start_frame;
        start_frame.can_id = 0x000; // NMT 广播 ID
        start_frame.can_dlc = 8;
        std::memset(start_frame.data, 0, 8);
        start_frame.data[0] = 0x01; // 0x01 代表“启动”节点通信
        start_frame.data[1] = 0x00; // 0x00 代表广播给所有节点
        
        if (write(can_socket_, &start_frame, sizeof(start_frame)) < 0) {
            RCLCPP_WARN(this->get_logger(), "唤醒指令发送失败，请检查 CAN 连接状态");
        } else {
            RCLCPP_INFO(this->get_logger(), "已成功发送水深传感器唤醒指令 (NMT Start)");
        }
    }

    // 看门狗回调逻辑
    void watchdog_callback() {
        if (!is_running_) return;

        uint64_t current_time = this->now().nanoseconds();
        uint64_t last_time;
        {
            std::lock_guard<std::mutex> lock(msg_mutex_);
            last_time = cached_msg_.timestamp;
        }

        // 判断：从未收到数据(0)，或者距离上次收到数据超过 2 秒 (2,000,000,000 纳秒)
        if (last_time == 0 || (current_time - last_time) > 2000000000ULL) {
            
            // 【新增状态逻辑】如果之前是连接状态，现在断开了，只触发一次发布
            if (is_connected_) {
                is_connected_ = false;
                publish_status("DISCONNECTED");
                RCLCPP_ERROR(this->get_logger(), "触发看门狗！未收到水深数据超时，传感器已断开。");
            }
            
            // 下方保留原有的尝试重连机制
            if (can_socket_ >= 0) {
                close(can_socket_);
                can_socket_ = -1;
            }
            
            // 重新拉起套接字
            std::string can_iface = this->get_parameter("can_interface").as_string();
            can_socket_ = setup_can_socket(can_iface);
            
            // 重新发送唤醒报文
            if (can_socket_ >= 0) {
                send_nmt_wakeup();
            }
        }
    }

    void publish_timer_callback() {
        if (depth_pub_->is_activated()) {
            hal::msg::HalDepthsensor msg_to_publish;
            {
                std::lock_guard<std::mutex> lock(msg_mutex_);
                msg_to_publish = cached_msg_;
            }
            // 只有在获取到时间戳（说明收到过CAN数据）后才发布
            if (msg_to_publish.timestamp != 0) {
                depth_pub_->publish(msg_to_publish);
            }
        }
    }

    void can_thread_function() {
        struct can_frame frame;

        while (rclcpp::ok() && is_running_) {
            if (can_socket_ < 0) {
                std::this_thread::sleep_for(1s);
                continue;
            }

            // 使用 select 机制替代忙轮询，防止阻塞和死锁
            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(can_socket_, &read_fds);

            struct timeval tv;
            tv.tv_sec = 0;
            tv.tv_usec = 10000; // 10ms 超时响应

            int ret = select(can_socket_ + 1, &read_fds, NULL, NULL, &tv);

            // ret > 0 说明有数据可读
            if (ret > 0 && FD_ISSET(can_socket_, &read_fds)) {
                int nbytes = read(can_socket_, &frame, sizeof(struct can_frame));
                
                if (nbytes == sizeof(struct can_frame)) {
                    // 1. 过滤无效长度
                    if (frame.can_dlc < 8) continue; 

                    // 2. 匹配真实的 CANOpen PDO ID (0x181 为1号节点，0x182 为2号节点)
                    if (frame.can_id != 0x181 && frame.can_id != 0x182) {
                        continue; 
                    }

                    // 3. 按协议提取为 32 位有符号整数 (小端模式拼接)
                    int32_t raw_pressure = (frame.data[3] << 24) | (frame.data[2] << 16) | (frame.data[1] << 8) | frame.data[0];
                    int32_t raw_temp     = (frame.data[7] << 24) | (frame.data[6] << 16) | (frame.data[5] << 8) | frame.data[4];

                    // 4. 物理换算 (根据说明书及老代码常量)
                    float depth_val = (float)raw_pressure * 100.0f / (9.8f * 1000.0f);
                    float temp_val  = (float)raw_temp / 1000.0f;

                    // 安全过滤：防止传感器异常发来 NaN 或 无穷大
                    if (std::isnan(depth_val) || std::isinf(depth_val)) {
                        continue;
                    }

                    // 【新增状态逻辑】只要成功读出合法数据，就检查并恢复连接状态
                    if (!is_connected_) {
                        is_connected_ = true;
                        publish_status("CONNECTED");
                        RCLCPP_INFO(this->get_logger(), "传感器已恢复连接，收到有效数据。");
                    }

                    // 5. 获取锁，更新缓存，供 50Hz 定时器发布
                    {
                        std::lock_guard<std::mutex> lock(msg_mutex_);
                        cached_msg_.timestamp = this->now().nanoseconds();
                        
                        if (frame.can_id == 0x181) {
                            cached_msg_.depth_1 = depth_val;
                            cached_msg_.temp_1  = temp_val;
                            has_data_1_ = true;
                        } else if (frame.can_id == 0x182) {
                            cached_msg_.depth_2 = depth_val;
                            cached_msg_.temp_2  = temp_val;
                            has_data_2_ = true;
                        }

                        // 计算平均深度：用布尔标志位判断，取代原有的 0 值判断，避免零漂在空气中为负数时的逻辑错误
                        if (has_data_1_ && has_data_2_) {
                            cached_msg_.depth_avg = (cached_msg_.depth_1 + cached_msg_.depth_2) / 2.0f;
                        } else if (has_data_1_) {
                            cached_msg_.depth_avg = cached_msg_.depth_1;
                        } else if (has_data_2_) {
                            cached_msg_.depth_avg = cached_msg_.depth_2;
                        }
                    }
                }
            }
        }
    }
};

int main(int argc, char ** argv) {
    rclcpp::init(argc, argv);
    // 使用 std::make_shared 创建并自动管理节点生命周期
    auto node = std::make_shared<HalDepthSensorNode>("hal_depthsensor_node");
    rclcpp::spin(node->get_node_base_interface());
    rclcpp::shutdown();
    return 0;
}