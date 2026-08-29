#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <cstring>
#include <mutex>
#include <atomic>
#include <fcntl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <cerrno>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "rclcpp_lifecycle/lifecycle_publisher.hpp"
#include "std_msgs/msg/float64_multi_array.hpp"
#include "lifecycle_msgs/msg/state.hpp"

#include "hal/msg/hal_tailservo.hpp"
#include "hal/msg/hal_wingservo.hpp"
#include "hal/srv/hal_servocontrol_srv.hpp"

using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;
using std::placeholders::_1;
using std::placeholders::_2;

class HalServoNode : public rclcpp_lifecycle::LifecycleNode {
public:
    explicit HalServoNode(const std::string & node_name, bool intra_process_comms = false)
    : rclcpp_lifecycle::LifecycleNode(node_name,
        rclcpp::NodeOptions().use_intra_process_comms(intra_process_comms)) {}

    CallbackReturn on_configure(const rclcpp_lifecycle::State &) override {
        RCLCPP_INFO(get_logger(), "配置中... 初始化硬件接口与多线程回调组。");
        
        max_angle_ = 45.0;
        
        // ==========================================
        // 1. 创建独立的互斥回调组
        // ==========================================
        timer_cb_group_ = this->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
        sub_cb_group_ = this->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
        srv_cb_group_ = this->create_callback_group(rclcpp::CallbackGroupType::MutuallyExclusive);
        
        // ==========================================
        // 2. 初始化发布者
        // ==========================================
        pub_tail_status_ = this->create_publisher<hal::msg::HalTailservo>("/hal/tailservo", 10);
        pub_wing_status_ = this->create_publisher<hal::msg::HalWingservo>("/hal/wingservo", 10);
        
        // ==========================================
        // 3. 初始化服务 (绑定到 srv_cb_group_)
        // ==========================================
        srv_control_ = this->create_service<hal::srv::HalServocontrolSrv>(
            "/hal/servocontrol",
            std::bind(&HalServoNode::control_srv_callback, this, std::placeholders::_1, std::placeholders::_2),
            rmw_qos_profile_services_default,
            srv_cb_group_);
        
        // ==========================================
        // 4. 初始化订阅者 (绑定到 sub_cb_group_)
        // ==========================================
        auto sub_opt = rclcpp::SubscriptionOptions();
        sub_opt.callback_group = sub_cb_group_;
        
        sub_tail_cmd_ = this->create_subscription<std_msgs::msg::Float64MultiArray>(
            "/hal/servo/tail_cmd", 10, 
            std::bind(&HalServoNode::tail_cmd_callback, this, std::placeholders::_1), 
            sub_opt);
        
        sub_wing_cmd_ = this->create_subscription<std_msgs::msg::Float64MultiArray>(
            "/hal/servo/wing_cmd", 10, 
            std::bind(&HalServoNode::wing_cmd_callback, this, std::placeholders::_1), 
            sub_opt);
        
        // ==========================================
        // 5. 初始化定时器 (绑定到 timer_cb_group_)
        // ==========================================
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(4),
            std::bind(&HalServoNode::timer_publish_status_callback, this),
            timer_cb_group_);
        timer_->cancel(); // 默认挂起，避免 Inactive 状态空转
        
        // ==========================================
        // 6. 初始化硬件与后台读取线程
        // ==========================================
        hardware_api_init();
        keep_reading_ = true;
        read_thread_ = std::thread(&HalServoNode::serial_read_loop, this);
        
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_activate(const rclcpp_lifecycle::State & state) override {
        RCLCPP_INFO(get_logger(), "舵机节点已激活。");
        pub_tail_status_->on_activate();
        pub_wing_status_->on_activate();
        
        // [新增] 激活时重置看门狗时间戳，防止刚启动就误报断联
        last_valid_rx_time_.store(this->get_clock()->now().nanoseconds());
        
        timer_->reset(); // 激活状态才开启定时器
        is_testing_ = false;
        LifecycleNode::on_activate(state);
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_deactivate(const rclcpp_lifecycle::State & state) override {
        RCLCPP_INFO(get_logger(), "舵机节点停用，触发急停：释放所有舵机力矩。");
        timer_->cancel(); // 停用状态挂起定时器
        
        is_testing_ = false; // 触发自检退出
        if (test_thread_.joinable()) test_thread_.join();

        pub_tail_status_->on_deactivate();
        pub_wing_status_->on_deactivate();
        send_stop_command_to_all(false); // 释放锁力
        LifecycleNode::on_deactivate(state);
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_cleanup(const rclcpp_lifecycle::State &) override {
        keep_reading_ = false;
        is_testing_ = false;
        if (read_thread_.joinable()) read_thread_.join();
        if (test_thread_.joinable()) test_thread_.join();

        pub_tail_status_.reset(); pub_wing_status_.reset();
        srv_control_.reset(); sub_tail_cmd_.reset(); sub_wing_cmd_.reset(); timer_.reset();

        if (serial_fd_ >= 0) {
            close(serial_fd_);
            serial_fd_ = -1;
        }
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_shutdown(const rclcpp_lifecycle::State &) override {
        is_testing_ = false;
        if (test_thread_.joinable()) test_thread_.join();
        send_stop_command_to_all(false); // 释放锁力
        return CallbackReturn::SUCCESS;
    }

private:
    double max_angle_;
    std::atomic<bool> is_testing_{false};
    
    // [新增] 软件看门狗时间戳
    std::atomic<int64_t> last_valid_rx_time_{0};

    uint8_t current_poll_id_ = 0;

    // 串口与线程
    int serial_fd_ = -1;
    std::mutex serial_write_mutex_;
    std::thread read_thread_;
    std::thread test_thread_; 
    std::atomic<bool> keep_reading_{false};

    // 数据缓存池
    std::mutex data_cache_mutex_;
    hal::msg::HalTailservo cached_tail_msg_;
    hal::msg::HalWingservo cached_wing_msg_;

    // ROS 2 接口
    std::shared_ptr<rclcpp_lifecycle::LifecyclePublisher<hal::msg::HalTailservo>> pub_tail_status_;
    std::shared_ptr<rclcpp_lifecycle::LifecyclePublisher<hal::msg::HalWingservo>> pub_wing_status_;
    std::shared_ptr<rclcpp::Service<hal::srv::HalServocontrolSrv>> srv_control_;
    rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr sub_tail_cmd_;
    rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr sub_wing_cmd_;
    rclcpp::TimerBase::SharedPtr timer_;

    rclcpp::CallbackGroup::SharedPtr timer_cb_group_;
    rclcpp::CallbackGroup::SharedPtr sub_cb_group_;
    rclcpp::CallbackGroup::SharedPtr srv_cb_group_;

    // ==========================================
    // ⚙️ 通信协议引擎
    // ==========================================
    std::vector<uint8_t> pack_command(uint8_t cmd_id, const std::vector<uint8_t>& content) {
        std::vector<uint8_t> packet;
        packet.push_back(0x12); packet.push_back(0x4C);
        packet.push_back(cmd_id);
        packet.push_back(static_cast<uint8_t>(content.size()));
        packet.insert(packet.end(), content.begin(), content.end());
        uint32_t sum = 0;
        for (uint8_t b : packet) sum += b;
        packet.push_back(static_cast<uint8_t>(sum % 256));
        return packet;
    }

    void hardware_serial_write(const std::vector<uint8_t>& data) {
        std::lock_guard<std::mutex> lock(serial_write_mutex_);
        if (serial_fd_ < 0) return;

        // 设置目标 CAN ID 为 10
        uint32_t TARGET_CAN_ID = 0x10; 

        size_t bytes_sent = 0;
        while (bytes_sent < data.size()) {
            struct can_frame frame;
            memset(&frame, 0, sizeof(frame));
            frame.can_id = TARGET_CAN_ID;
            
            // 计算当前帧的发包长度 (不能超过 CAN 标准的 8 字节)
            size_t chunk_size = std::min(static_cast<size_t>(8), data.size() - bytes_sent);
            frame.can_dlc = chunk_size;

            std::memcpy(frame.data, data.data() + bytes_sent, chunk_size);
            write(serial_fd_, &frame, sizeof(struct can_frame));
            
            bytes_sent += chunk_size;
        }
    }

    // ==========================================
    // �� 后台读取线程
    // ==========================================
    void serial_read_loop() {
        int state = 0;
        uint8_t cmd_id = 0, length = 0, checksum = 0;
        std::vector<uint8_t> payload;
        
        // 期望接收的 CAN ID
        uint32_t EXPECTED_CAN_ID = 0x10;

        while (keep_reading_) {
            struct can_frame frame;
            int bytes_read = read(serial_fd_, &frame, sizeof(struct can_frame));
            
            if (bytes_read == sizeof(struct can_frame)) {
                // 如果总线杂乱，可以过滤掉不是模块发来的数据
                // if (frame.can_id != EXPECTED_CAN_ID) continue;

                // 拆解 CAN 帧数据，喂给原有状态机
                for (int i = 0; i < frame.can_dlc; ++i) {
                    uint8_t byte = frame.data[i];
                    
                    switch(state) {
                        case 0: if (byte == 0x05) state = 1; break;
                        case 1: if (byte == 0x1C) state = 2; else state = (byte == 0x05) ? 1 : 0; break;
                        case 2: cmd_id = byte; state = 3; break;
                        case 3: 
                            length = byte; 
                            if (length > 64) { state = 0; break; }
                            payload.clear(); 
                            state = (length > 0) ? 4 : 5; 
                            break;
                        case 4:
                            payload.push_back(byte);
                            if (payload.size() == length) state = 5;
                            break;
                        case 5:
                            checksum = byte;
                            uint32_t sum = 0x05 + 0x1C + cmd_id + length;
                            for (uint8_t b : payload) sum += b;
                            if ((sum % 256) == checksum) process_servo_response(cmd_id, payload);
                            state = 0;
                            break;
                    }
                }
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(500));
            }
        }
    }

    void process_servo_response(uint8_t cmd_id, const std::vector<uint8_t>& payload) {
        if (cmd_id == 0x0A && payload.size() >= 3) {
            
            // [新增] 喂狗：只要成功解包一帧正确的状态反馈，就更新时间戳
            last_valid_rx_time_.store(this->get_clock()->now().nanoseconds());

            uint8_t servo_id = payload[0];
            int16_t pos_raw = static_cast<int16_t>((payload[2] << 8) | payload[1]);
            float angle = static_cast<float>(pos_raw) / 10.0f;
    
            std::lock_guard<std::mutex> lock(data_cache_mutex_);
            if (servo_id <= 3) {
                cached_tail_msg_.position[servo_id] = angle;
            } else if (servo_id >= 4 && servo_id <= 5) {
                cached_wing_msg_.position[servo_id - 4] = angle;
            }
        }
    }

    // ==========================================
    // ⏱️ 定时器：轮询硬件并发布 Topic
    // ==========================================
    void timer_publish_status_callback() {
        if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) return;

        // 无论如何，定时下发探寻指令
        hardware_serial_write(pack_command(0x0A, {current_poll_id_}));
        current_poll_id_ = (current_poll_id_ + 1) % 6;

        if (current_poll_id_ == 0) {
            int64_t current_timestamp = this->get_clock()->now().nanoseconds();

            // [核心] 看门狗拦截：判断是否超过 100ms 未收到数据 (100,000,000 纳秒)
            if (current_timestamp - last_valid_rx_time_.load() > 100000000) {
                RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000, 
                    "⚠️ 严重警告：总线通信超时！舵机疑似掉线或断电。[看门狗触发] 已停止发布僵尸数据。");
                // 方案 A：直接 return，切断 Topic 发布。上层节点将因为订阅超时而触发 Failsafe
                return; 
            }

            // 数据依然新鲜，正常发布
            {
                std::lock_guard<std::mutex> lock(data_cache_mutex_);
                cached_tail_msg_.timestamp = current_timestamp;
                pub_tail_status_->publish(cached_tail_msg_);

                cached_wing_msg_.timestamp = current_timestamp;
                pub_wing_status_->publish(cached_wing_msg_);
            }
        }
    }

    // ==========================================
    // �� 控制逻辑 (Service & CMD)
    // ==========================================
    void send_angle_command(uint8_t id, double deg) {
        int16_t angle_raw = static_cast<int16_t>(deg * 10.0);
        uint16_t time = 500, power = 1000;
        std::vector<uint8_t> c = {id};
        c.push_back(angle_raw & 0xFF); c.push_back((angle_raw >> 8) & 0xFF);
        c.push_back(time & 0xFF);      c.push_back((time >> 8) & 0xFF);
        c.push_back(power & 0xFF);     c.push_back((power >> 8) & 0xFF);
        hardware_serial_write(pack_command(0x08, c));
    }

    void send_stop_command_to_all(bool lock) {
        uint8_t val = lock ? 0x01 : 0x00;
        for (uint8_t id = 0; id <= 5; ++id) hardware_serial_write(pack_command(0x18, {id, val}));
    }

    void tail_cmd_callback(const std_msgs::msg::Float64MultiArray::SharedPtr msg) {
        if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE || is_testing_) return;
    
        if (msg->data.size() >= 4) {
            for (int i=0; i<4; ++i) {
                send_angle_command(i, std::max(-max_angle_, std::min(msg->data[i], max_angle_)));
            }
        }
    }
    
    void wing_cmd_callback(const std_msgs::msg::Float64MultiArray::SharedPtr msg) {
        if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE || is_testing_) return;
        
        if (msg->data.size() >= 2) {
            for (int i=0; i<2; ++i) send_angle_command(i + 4, std::max(-max_angle_, std::min(msg->data[i], max_angle_)));
        }
    }

    // 对应协议 #34 的服务回调
    void control_srv_callback(const std::shared_ptr<hal::srv::HalServocontrolSrv::Request> req,
                              std::shared_ptr<hal::srv::HalServocontrolSrv::Response> res) {
        if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) {
            res->success = false; res->message = "节点未就绪（非Active状态）"; return;
        }

        uint8_t cmd = req->command;
        if (cmd == 0x01 || cmd == 0x02) {
            // 原子操作锁定执行状态，防止连续下发请求导致线程 join 死锁
            bool expected = false;
            if (!is_testing_.compare_exchange_strong(expected, true)) {
                res->success = false; res->message = "当前舵机正在执行自检序列，拒绝重入"; return;
            }

            if (test_thread_.joinable()) test_thread_.join(); 
            
            test_thread_ = std::thread(&HalServoNode::execute_test_sequence, this, cmd == 0x01);
            res->success = true; res->message = "舵机自检测试已成功异步启动";
        } else {
            res->success = false; res->message = "无效的协议指令代码";
        }
    }

    void execute_test_sequence(bool is_tail) {
        double target_test_angle = 15.0; 
        
        auto set_angles = [&](double a) { 
            if(is_tail) {
                for(int i=0; i<=3; ++i) {
                    send_angle_command(i, a); 
                    // [核心修复 1] 帧间缓冲延时。给单片机喘息时间，防止连续发包冲爆 MCU 的串口中断
                    std::this_thread::sleep_for(std::chrono::milliseconds(4)); 
                }
            } else {
                for(int i=4; i<=5; ++i) {
                    send_angle_command(i, a); 
                    std::this_thread::sleep_for(std::chrono::milliseconds(4));
                }
            }
        };

        if (!keep_reading_ || !is_testing_) { is_testing_ = false; return; }

        // 1. 正转
        set_angles(target_test_angle);  
        if (!interruptible_sleep(3000)) { is_testing_ = false; return; }

        // 2. 反转
        set_angles(-target_test_angle); 
        if (!interruptible_sleep(3000)) { is_testing_ = false; return; }

        // 3. 停机恢复回中
        set_angles(0.0); 
        is_testing_ = false;
        RCLCPP_INFO(get_logger(), "协议 #34 规定的舵机自检序列执行完毕。");
    }

    bool interruptible_sleep(int milliseconds) {
        int elapsed = 0;
        while (elapsed < milliseconds) {
            // 原有的生命周期打断
            if (!keep_reading_ || !is_testing_) return false;
            
            // [核心修复 2] 将看门狗与自检线程联动
            int64_t current_timestamp = this->get_clock()->now().nanoseconds();
            if (current_timestamp - last_valid_rx_time_.load() > 100000000) {
                RCLCPP_ERROR(this->get_logger(), "自检强行中止：检测到舵机物理掉线或电源电压跌落！");
                return false; // 立刻打断这 3000 毫秒的死等
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            elapsed += 50;
        }
        return true;
    }

    // ==========================================
    // ⚙️ 硬件底层接口
    // ==========================================
    void hardware_api_init() {
        std::string can_interface = "can3"; // 主控板的 CAN 接口
        
        serial_fd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
        if (serial_fd_ < 0) {
            RCLCPP_ERROR(get_logger(), "无法创建 CAN Socket: %s", strerror(errno));
            return;
        }

        struct ifreq ifr;
        strcpy(ifr.ifr_name, can_interface.c_str());
        ioctl(serial_fd_, SIOCGIFINDEX, &ifr);

        struct sockaddr_can addr;
        memset(&addr, 0, sizeof(addr));
        addr.can_family = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;

        // 设置为非阻塞模式，防止读取线程死锁c
        int flags = fcntl(serial_fd_, F_GETFL, 0);
        fcntl(serial_fd_, F_SETFL, flags | O_NONBLOCK);

        if (bind(serial_fd_, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            RCLCPP_ERROR(get_logger(), "无法绑定 CAN 接口 %s: %s", can_interface.c_str(), strerror(errno));
            close(serial_fd_);
            serial_fd_ = -1;
            return;
        }
        
        RCLCPP_INFO(get_logger(), "SocketCAN 接口 %s 初始化成功，通信目标 ID: 10", can_interface.c_str());
    }

    void set_tail_servos_hardware(const std::vector<double>& a) { for(size_t i=0; i<a.size(); ++i) send_angle_command(i, a[i]); }
    void set_wing_servos_hardware(const std::vector<double>& a) { for(size_t i=0; i<a.size(); ++i) send_angle_command(i+4, a[i]); }
};

int main(int argc, char ** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<HalServoNode>("hal_servo_node");

    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(node->get_node_base_interface());

    RCLCPP_INFO(node->get_logger(), "已启动多线程执行器，正在监听舵机总线与服务指令...");
    executor.spin();

    rclcpp::shutdown();
    return 0;
}