#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include "hal/msg/hal_depthsensor_msg.hpp" 
#include <cmath> 

#include <cmath>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <unistd.h>
#include <string.h>
#include <thread>
#include <mutex>
#include <chrono>

using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;
using namespace std::chrono_literals;

class HalDepthSensorNode : public rclcpp_lifecycle::LifecycleNode
{
public:
    HalDepthSensorNode(const std::string & node_name)
    : rclcpp_lifecycle::LifecycleNode(node_name)
    {
        this->declare_parameter<std::string>("can_interface", "can0");
    }

    CallbackReturn on_configure(const rclcpp_lifecycle::State &) override {
        RCLCPP_INFO(get_logger(), "配置中... 初始化水深传感器发布者 (固定 50Hz)。");
        depth_pub_ = this->create_publisher<hal::msg::HalDepthsensorMsg>("hal_depthsensor_msg", 10);
        
        // 【新增】创建 50Hz (20ms) 定时器，专门负责发布
        publish_timer_ = this->create_wall_timer(
            20ms, std::bind(&HalDepthSensorNode::publish_timer_callback, this));
            
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_activate(const rclcpp_lifecycle::State & state) override {
        depth_pub_->on_activate();
        std::string can_iface = this->get_parameter("can_interface").as_string();
        
        can_socket_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
        if (can_socket_ < 0) return CallbackReturn::FAILURE;

        struct ifreq ifr;
        strcpy(ifr.ifr_name, can_iface.c_str());
        ioctl(can_socket_, SIOCGIFINDEX, &ifr);

        struct sockaddr_can addr;
        memset(&addr, 0, sizeof(addr));
        addr.can_family = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;

        if (bind(can_socket_, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            close(can_socket_);
            return CallbackReturn::FAILURE;
        }

        int timestamp_on = 1;
        setsockopt(can_socket_, SOL_SOCKET, SO_TIMESTAMP, &timestamp_on, sizeof(timestamp_on));

        is_running_ = true;
        can_thread_ = std::thread(&HalDepthSensorNode::can_thread_function, this);
        
        return LifecycleNode::on_activate(state);
    }

    CallbackReturn on_deactivate(const rclcpp_lifecycle::State & state) override {
        depth_pub_->on_deactivate();
        is_running_ = false;
        if (can_thread_.joinable()) can_thread_.join();
        if (can_socket_ >= 0) { close(can_socket_); can_socket_ = -1; }
        return LifecycleNode::on_deactivate(state);
    }

    CallbackReturn on_cleanup(const rclcpp_lifecycle::State &) override {
        depth_pub_.reset();
        publish_timer_.reset();
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_shutdown(const rclcpp_lifecycle::State &) override {
        is_running_ = false;
        if (can_thread_.joinable()) can_thread_.join();
        if (can_socket_ >= 0) close(can_socket_);
        return CallbackReturn::SUCCESS;
    }

private:
    std::shared_ptr<rclcpp_lifecycle::LifecyclePublisher<hal::msg::HalDepthsensorMsg>> depth_pub_;
    rclcpp::TimerBase::SharedPtr publish_timer_; // 【新增】发布定时器
    
    hal::msg::HalDepthsensorMsg cached_msg_;     // 【新增】数据缓存
    std::mutex msg_mutex_;                       // 【新增】互斥锁，保护缓存
    
    int can_socket_ = -1;
    std::thread can_thread_;
    std::atomic<bool> is_running_{false};

    // 【新增】定时器回调：以严格的 50Hz 频率执行
    void publish_timer_callback() {
        if (depth_pub_->is_activated()) {
            hal::msg::HalDepthsensorMsg msg_to_publish;
            {
                // 加锁，将底层线程刚写入的最新数据拷贝出来，然后迅速解锁
                std::lock_guard<std::mutex> lock(msg_mutex_);
                msg_to_publish = cached_msg_;
            }
            // 发布拷贝出的数据（此时时间戳仍是硬件底层赋予的精准时间）
            depth_pub_->publish(msg_to_publish);
        }
    }
void can_thread_function() {
        struct msghdr msg;
        struct iovec iov;
        struct can_frame frame;
        char ctrlmsg[CMSG_SPACE(sizeof(struct timeval))]; 

        iov.iov_base = &frame;
        iov.iov_len = sizeof(frame);
        msg.msg_name = NULL; msg.msg_namelen = 0; msg.msg_iov = &iov; msg.msg_iovlen = 1;
        msg.msg_control = &ctrlmsg; msg.msg_controllen = sizeof(ctrlmsg); msg.msg_flags = 0;

        float temp_d1 = 0.0f, temp_d2 = 0.0f;

        while (rclcpp::ok() && is_running_) {
            // 1. 【新增】断线自动重连机制
            if (can_socket_ < 0) {
                // 假设你有一个初始化 CAN 的函数，例如 setup_can_socket()
                // can_socket_ = setup_can_socket("can0");
                if (can_socket_ < 0) {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                    continue; // 重连失败则继续等待
                }
                RCLCPP_INFO(this->get_logger(), "CAN Socket 重新连接成功!");
            }

            // 2. 原生 Socket 错误不抛异常，需检查返回值
            int nbytes = recvmsg(can_socket_, &msg, 0);
            
            if (nbytes < 0) {
                // 非阻塞模式下没数据属于正常情况
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(2));
                    continue;
                }
                // 其他情况视为总线错误/断开
                RCLCPP_ERROR(this->get_logger(), "CAN 接收错误: %s", strerror(errno));
                close(can_socket_);
                can_socket_ = -1; // 触发下一次循环的重连
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }

            if (nbytes > 0) {
                // 【核心修复】必须校验 CAN 数据长度 (DLC)，防止 memcpy 越界读取垃圾内存
                if (frame.can_dlc < sizeof(float)) {
                    RCLCPP_DEBUG(this->get_logger(), "收到残缺 CAN 帧，已丢弃");
                    continue;
                }

                int64_t can_timestamp_ns = 0;
                struct cmsghdr *cmsg;
                struct timeval tv;
                for (cmsg = CMSG_FIRSTHDR(&msg); cmsg != NULL; cmsg = CMSG_NXTHDR(&msg, cmsg)) {
                    if (cmsg->cmsg_level == SOL_SOCKET && cmsg->cmsg_type == SO_TIMESTAMP) {
                        memcpy(&tv, CMSG_DATA(cmsg), sizeof(tv));
                        can_timestamp_ns = tv.tv_sec * 1000000000ULL + tv.tv_usec * 1000ULL;
                        break;
                    }
                }
                if (can_timestamp_ns == 0) can_timestamp_ns = this->now().nanoseconds();

                // 提取浮点数数据
                float parsed_val = 0.0f;
                std::memcpy(&parsed_val, frame.data, sizeof(float));

                // 【新增】安全过滤：防止传感器死机发来 NaN 或 无穷大数据引发控制灾难
                if (std::isnan(parsed_val) || std::isinf(parsed_val)) {
                    RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 2000, "水深传感器发来无效浮点数(NaN/Inf)");
                    continue;
                }

                if (frame.can_id == 0x101) {
                    temp_d1 = parsed_val;
                } else if (frame.can_id == 0x102) {
                    temp_d2 = parsed_val;
                } else {
                    continue; // 不是水深数据包，跳过后续加锁，节省性能
                }

                // 获取锁，更新缓存，供 50Hz 定时器发布
                {
                    std::lock_guard<std::mutex> lock(msg_mutex_);
                    cached_msg_.depth_1 = temp_d1;
                    cached_msg_.depth_2 = temp_d2;
                    cached_msg_.depth_avg = (temp_d1 > 0 && temp_d2 > 0) ? (temp_d1 + temp_d2) / 2.0f : std::max(temp_d1, temp_d2);
                    cached_msg_.timestamp = can_timestamp_ns; 
                }
            }
        }
    }
};

int main(int argc, char ** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<HalDepthSensorNode>("hal_depthsensor_node")->get_node_base_interface());
    rclcpp::shutdown();
    return 0;
}