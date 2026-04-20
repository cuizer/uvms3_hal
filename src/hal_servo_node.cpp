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
#include <termios.h>
#include <unistd.h>
#include <cerrno>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "rclcpp_lifecycle/lifecycle_publisher.hpp"
#include "std_msgs/msg/float64_multi_array.hpp"
#include "lifecycle_msgs/msg/state.hpp"

// 🚨 请确保你的 .msg 文件已更新包含 timestamp, position, turns 字段
#include "hal/msg/hal_tailservo_msg.hpp"
#include "hal/msg/hal_wingservo_msg.hpp"
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
        RCLCPP_INFO(get_logger(), "配置中... 初始化硬件接口与后台线程。");

        max_angle_ = 45.0;

        // 1. 设置发布者 (符合表格定义)
        pub_tail_status_ = this->create_publisher<hal::msg::HalTailservoMsg>("/hal/tailservo", 10);
        pub_wing_status_ = this->create_publisher<hal::msg::HalWingservoMsg>("/hal/wingservo", 10);

        // 2. 设置服务 (按照你的要求保留 Service)
        srv_control_ = this->create_service<hal::srv::HalServocontrolSrv>(
            "hal_servocontrol_srv",
            std::bind(&HalServoNode::control_srv_callback, this, _1, _2));

        // 3. 设置订阅者
        sub_tail_cmd_ = this->create_subscription<std_msgs::msg::Float64MultiArray>(
            "/hal/servo/tail_cmd", 10, std::bind(&HalServoNode::tail_cmd_callback, this, _1));
        sub_wing_cmd_ = this->create_subscription<std_msgs::msg::Float64MultiArray>(
            "/hal/servo/wing_cmd", 10, std::bind(&HalServoNode::wing_cmd_callback, this, _1));

        // 4. 初始化硬件
        hardware_api_init();

        // 5. 启动后台读取线程
        keep_reading_ = true;
        read_thread_ = std::thread(&HalServoNode::serial_read_loop, this);

        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(20), // 50Hz 轮询数据并发布
            std::bind(&HalServoNode::timer_publish_status_callback, this));

        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_activate(const rclcpp_lifecycle::State & state) override {
        RCLCPP_INFO(get_logger(), "舵机节点已激活。");
        pub_tail_status_->on_activate();
        pub_wing_status_->on_activate();
        is_testing_ = false;
        LifecycleNode::on_activate(state);
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_deactivate(const rclcpp_lifecycle::State & state) override {
        RCLCPP_INFO(get_logger(), "舵机节点停用，舵机强制回中。");
        pub_tail_status_->on_deactivate();
        pub_wing_status_->on_deactivate();
        set_tail_servos_hardware({0.0, 0.0, 0.0, 0.0});
        set_wing_servos_hardware({0.0, 0.0});
        send_stop_command_to_all(true); // 保持锁力
        LifecycleNode::on_deactivate(state);
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_cleanup(const rclcpp_lifecycle::State &) override {
        keep_reading_ = false;
        if (read_thread_.joinable()) read_thread_.join();

        pub_tail_status_.reset(); pub_wing_status_.reset();
        srv_control_.reset(); sub_tail_cmd_.reset(); sub_wing_cmd_.reset(); timer_.reset();

        if (serial_fd_ >= 0) {
            close(serial_fd_);
            serial_fd_ = -1;
        }
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_shutdown(const rclcpp_lifecycle::State &) override {
        send_stop_command_to_all(false); // 释放锁力
        return CallbackReturn::SUCCESS;
    }

private:
    double max_angle_;
    std::atomic<bool> is_testing_{false};

    // 串口与线程
    int serial_fd_ = -1;
    std::mutex serial_write_mutex_;
    std::thread read_thread_;
    std::atomic<bool> keep_reading_{false};

    // 数据缓存池
    std::mutex data_cache_mutex_;
    hal::msg::HalTailservoMsg cached_tail_msg_;
    hal::msg::HalWingservoMsg cached_wing_msg_;

    // ROS 2 接口
    std::shared_ptr<rclcpp_lifecycle::LifecyclePublisher<hal::msg::HalTailservoMsg>> pub_tail_status_;
    std::shared_ptr<rclcpp_lifecycle::LifecyclePublisher<hal::msg::HalWingservoMsg>> pub_wing_status_;
    std::shared_ptr<rclcpp::Service<hal::srv::HalServocontrolSrv>> srv_control_;
    rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr sub_tail_cmd_;
    rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr sub_wing_cmd_;
    rclcpp::TimerBase::SharedPtr timer_;

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
        write(serial_fd_, data.data(), data.size());
        tcdrain(serial_fd_);
    }

    // ==========================================
    // 📡 后台读取线程 (支持回音屏蔽与角度解析)
    // ==========================================
    void serial_read_loop() {
        int state = 0;
        uint8_t cmd_id = 0, length = 0, checksum = 0;
        std::vector<uint8_t> payload;

        while (keep_reading_) {
            uint8_t byte;
            if (read(serial_fd_, &byte, 1) > 0) {
                switch(state) {
                    case 0: if (byte == 0x05) state = 1; break; // 寻找响应包头
                    case 1: if (byte == 0x1C) state = 2; else state = (byte == 0x05) ? 1 : 0; break;
                    case 2: cmd_id = byte; state = 3; break;
                    case 3: length = byte; payload.clear(); state = (length > 0) ? 4 : 5; break;
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
            } else {
                std::this_thread::sleep_for(std::chrono::microseconds(500));
            }
        }
    }

    void process_servo_response(uint8_t cmd_id, const std::vector<uint8_t>& payload) {
        if (cmd_id == 0x16 && payload.size() >= 16) {
            uint8_t servo_id = payload[0];
            // 📖 严格按照 Fashion Star 官方小端序解析
            uint16_t voltage = (payload[2] << 8) | payload[1];
            uint16_t current = (payload[4] << 8) | payload[3];
            uint16_t power   = (payload[6] << 8) | payload[5];
            uint16_t adc_temp = (payload[8] << 8) | payload[7];
            uint8_t  status  = payload[9];
            
            // 角度反馈 (4字节, 0.1°/单位, 小端序)
            uint32_t pos_raw = (payload[13] << 24) | (payload[12] << 16) | (payload[11] << 8) | payload[10];
            float angle = static_cast<float>(pos_raw) / 10.0f;

            // 圈数反馈 (2字节, 有符号 int16, 小端序)
            int16_t turns = static_cast<int16_t>((payload[15] << 8) | payload[14]);

            std::lock_guard<std::mutex> lock(data_cache_mutex_);
            if (servo_id >= 0 && servo_id <= 3) {
                int i = servo_id;
                cached_tail_msg_.voltage[i]     = voltage;
                cached_tail_msg_.current[i]     = current;
                cached_tail_msg_.power[i]       = power;
                cached_tail_msg_.temperature[i] = adc_temp;
                cached_tail_msg_.status[i]      = status;
                cached_tail_msg_.position[i]    = angle;
                cached_tail_msg_.turns[i]       = turns;
            } else if (servo_id >= 4 && servo_id <= 5) {
                int i = servo_id - 4;
                cached_wing_msg_.voltage[i]     = voltage;
                cached_wing_msg_.current[i]     = current;
                cached_wing_msg_.power[i]       = power;
                cached_wing_msg_.temperature[i] = adc_temp;
                cached_wing_msg_.status[i]      = status;
                cached_wing_msg_.position[i]    = angle;
                cached_wing_msg_.turns[i]       = turns;
            }
        }
    }

    // ==========================================
    // ⏱️ 定时器：轮询硬件并发布 Topic
    // ==========================================
    void timer_publish_status_callback() {
        if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) return;

        // 向所有 6 个舵机发送 0x16 状态请求
        for (uint8_t id = 0; id <= 5; ++id) {
            hardware_serial_write(pack_command(0x16, {id}));
            std::this_thread::sleep_for(std::chrono::milliseconds(2)); // 避免总线拥堵
        }

        int64_t current_timestamp = this->get_clock()->now().nanoseconds();
        {
            std::lock_guard<std::mutex> lock(data_cache_mutex_);
            cached_tail_msg_.timestamp = current_timestamp;
            pub_tail_status_->publish(cached_tail_msg_);

            cached_wing_msg_.timestamp = current_timestamp;
            pub_wing_status_->publish(cached_wing_msg_);
        }
    }

    // ==========================================
    // 🎮 控制逻辑 (Service & CMD)
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
            for (int i=0; i<4; ++i) send_angle_command(i+1, std::clamp(msg->data[i], -max_angle_, max_angle_));
        }
    }

    void wing_cmd_callback(const std_msgs::msg::Float64MultiArray::SharedPtr msg) {
        if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE || is_testing_) return;
        if (msg->data.size() >= 2) {
            for (int i=0; i<2; ++i) send_angle_command(i+5, std::clamp(msg->data[i], -max_angle_, max_angle_));
        }
    }

    void control_srv_callback(const std::shared_ptr<hal::srv::HalServocontrolSrv::Request> req,
                              std::shared_ptr<hal::srv::HalServocontrolSrv::Response> res) {
        if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE || is_testing_) {
            res->success = false; res->message = "节点未就绪或正在测试中"; return;
        }
        if (req->command == 0x01 || req->command == 0x02) {
            std::thread(&HalServoNode::execute_test_sequence, this, req->command == 0x01).detach();
            res->success = true; res->message = "测试已启动";
        } else {
            res->success = false; res->message = "无效指令";
        }
    }

    void execute_test_sequence(bool is_tail) {
        is_testing_ = true;
        auto set_angles = [&](double a){ if(is_tail) for(int i=0;i<=3;++i) send_angle_command(i,a); else for(int i=4;i<=5;++i) send_angle_command(i,a); };
        set_angles(3.0);  std::this_thread::sleep_for(std::chrono::seconds(2));
        set_angles(-3.0); std::this_thread::sleep_for(std::chrono::seconds(2));
        set_angles(0.0);  is_testing_ = false;
        RCLCPP_INFO(get_logger(), "测试完毕。");
    }

    void hardware_api_init() {
        // 如果你不用真实硬件，用虚拟串口软件模拟（推荐先用这个方案）：
        std::string port_name = "/dev/ttyTHS1";
        serial_fd_ = open(port_name.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
        if (serial_fd_ < 0) return;
        struct termios opt; tcgetattr(serial_fd_, &opt);
        cfsetispeed(&opt, B115200); cfsetospeed(&opt, B115200);
        opt.c_cflag |= (CLOCAL | CREAD | CS8); opt.c_cflag &= ~(PARENB | CSTOPB | CSIZE);
        opt.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
        opt.c_oflag &= ~OPOST; opt.c_cc[VMIN] = 0; opt.c_cc[VTIME] = 1;
        tcflush(serial_fd_, TCIFLUSH); tcsetattr(serial_fd_, TCSANOW, &opt);
    }

    void set_tail_servos_hardware(const std::vector<double>& a) { for(size_t i=0; i<a.size(); ++i) send_angle_command(i, a[i]); }
    void set_wing_servos_hardware(const std::vector<double>& a) { for(size_t i=0; i<a.size(); ++i) send_angle_command(i+4, a[i]); }
};

int main(int argc, char ** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<HalServoNode>("hal_servo_node");
    rclcpp::spin(node->get_node_base_interface());
    rclcpp::shutdown();
    return 0;
}