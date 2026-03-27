#include <algorithm>
#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "rclcpp_lifecycle/lifecycle_publisher.hpp"
#include "std_msgs/msg/float64_multi_array.hpp" 
#include "lifecycle_msgs/msg/state.hpp"

// 🚨 这里是真正的头文件！
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
        RCLCPP_INFO(get_logger(), "配置中... 初始化舵机节点接口。");

        max_angle_ = 45.0; 

        // 🚨 注意这里的命名空间全改成了 hal
        pub_tail_status_ = this->create_publisher<hal::msg::HalTailservoMsg>("hal_tailservo_msg", 10);
        pub_wing_status_ = this->create_publisher<hal::msg::HalWingservoMsg>("hal_wingservo_msg", 10);

        srv_control_ = this->create_service<hal::srv::HalServocontrolSrv>(
            "hal_servocontrol_srv",
            std::bind(&HalServoNode::control_srv_callback, this, _1, _2));

        sub_tail_cmd_ = this->create_subscription<std_msgs::msg::Float64MultiArray>(
            "/hal/servo/tail_cmd", 10, std::bind(&HalServoNode::tail_cmd_callback, this, _1));
        sub_wing_cmd_ = this->create_subscription<std_msgs::msg::Float64MultiArray>(
            "/hal/servo/wing_cmd", 10, std::bind(&HalServoNode::wing_cmd_callback, this, _1));

        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(20),
            std::bind(&HalServoNode::timer_publish_status_callback, this));

        hardware_api_init();
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
        
        LifecycleNode::on_deactivate(state);
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_cleanup(const rclcpp_lifecycle::State &) override {
        pub_tail_status_.reset(); pub_wing_status_.reset();
        srv_control_.reset(); sub_tail_cmd_.reset(); sub_wing_cmd_.reset(); timer_.reset();
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_shutdown(const rclcpp_lifecycle::State &) override {
        set_tail_servos_hardware({0.0, 0.0, 0.0, 0.0});
        set_wing_servos_hardware({0.0, 0.0});
        return CallbackReturn::SUCCESS;
    }

private:
    double max_angle_;
    bool is_testing_ = false; 

    // 🚨 这里的指针类型也都改成了 hal
    std::shared_ptr<rclcpp_lifecycle::LifecyclePublisher<hal::msg::HalTailservoMsg>> pub_tail_status_;
    std::shared_ptr<rclcpp_lifecycle::LifecyclePublisher<hal::msg::HalWingservoMsg>> pub_wing_status_;
    std::shared_ptr<rclcpp::Service<hal::srv::HalServocontrolSrv>> srv_control_;
    rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr sub_tail_cmd_;
    rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr sub_wing_cmd_;
    rclcpp::TimerBase::SharedPtr timer_;

    void control_srv_callback(
        const std::shared_ptr<hal::srv::HalServocontrolSrv::Request> request,
        std::shared_ptr<hal::srv::HalServocontrolSrv::Response> response) 
    {
        if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) {
            response->success = false; response->message = "节点未激活，无法执行舵机测试。"; return;
        }

        if (is_testing_) {
            response->success = false; response->message = "舵机正在测试中，请稍后再试。"; return;
        }

        uint8_t cmd = request->command;
        if (cmd == 0x01) {
            RCLCPP_INFO(get_logger(), "开始【尾部舵机】正反转 3° 测试...");
            std::thread(&HalServoNode::execute_test_sequence, this, true).detach();
            response->success = true; response->message = "尾舵测试序列已启动";
        } 
        else if (cmd == 0x02) {
            RCLCPP_INFO(get_logger(), "开始【翼部舵机】正反转 3° 测试...");
            std::thread(&HalServoNode::execute_test_sequence, this, false).detach();
            response->success = true; response->message = "翼舵测试序列已启动";
        } 
        else {
            response->success = false; response->message = "未知指令代码";
        }
    }

    void execute_test_sequence(bool is_tail_servo) {
        is_testing_ = true; 
        if (is_tail_servo) set_tail_servos_hardware({3.0, 3.0, 3.0, 3.0});
        else               set_wing_servos_hardware({3.0, 3.0});
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        if (is_tail_servo) set_tail_servos_hardware({-3.0, -3.0, -3.0, -3.0});
        else               set_wing_servos_hardware({-3.0, -3.0});
        std::this_thread::sleep_for(std::chrono::seconds(2));

        if (is_tail_servo) set_tail_servos_hardware({0.0, 0.0, 0.0, 0.0});
        else               set_wing_servos_hardware({0.0, 0.0});
        
        is_testing_ = false; 
        RCLCPP_INFO(get_logger(), "舵机测试序列执行完毕，恢复回中并开放控制。");
    }

    void tail_cmd_callback(const std_msgs::msg::Float64MultiArray::SharedPtr msg) {
        if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE || is_testing_) return;
        if (msg->data.size() >= 4) {
            std::vector<double> safe_cmds(4);
            for (int i = 0; i < 4; ++i) safe_cmds[i] = std::clamp(msg->data[i], -max_angle_, max_angle_);
            set_tail_servos_hardware(safe_cmds);
        }
    }

    void wing_cmd_callback(const std_msgs::msg::Float64MultiArray::SharedPtr msg) {
        if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE || is_testing_) return;
        if (msg->data.size() >= 2) {
            std::vector<double> safe_cmds(2);
            for (int i = 0; i < 2; ++i) safe_cmds[i] = std::clamp(msg->data[i], -max_angle_, max_angle_);
            set_wing_servos_hardware(safe_cmds);
        }
    }

    void timer_publish_status_callback() {
        if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) return;

        hal::msg::HalTailservoMsg tail_msg;
        for (int i = 0; i < 4; ++i) {
            tail_msg.voltage[i]     = hardware_read_tail_voltage(i);
            tail_msg.current[i]     = hardware_read_tail_current(i);
            tail_msg.power[i]       = hardware_read_tail_power(i);
            tail_msg.temperature[i] = hardware_read_tail_temp(i);
            tail_msg.status[i]      = 0x01; 
        }
        pub_tail_status_->publish(tail_msg);

        hal::msg::HalWingservoMsg wing_msg;
        for (int i = 0; i < 2; ++i) {
            wing_msg.voltage[i]     = hardware_read_wing_voltage(i);
            wing_msg.current[i]     = hardware_read_wing_current(i);
            wing_msg.power[i]       = hardware_read_wing_power(i);
            wing_msg.temperature[i] = hardware_read_wing_temp(i);
            wing_msg.status[i]      = 0x01;
        }
        pub_wing_status_->publish(wing_msg);
    }

    void hardware_api_init() {}
    void set_tail_servos_hardware(const std::vector<double>& /*angles*/) {}
    void set_wing_servos_hardware(const std::vector<double>& /*angles*/) {}
    int16_t hardware_read_tail_voltage(int) { return 2400; }
    int16_t hardware_read_tail_current(int) { return 150; } 
    uint16_t hardware_read_tail_power(int)  { return 36; }  
    uint16_t hardware_read_tail_temp(int)   { return 40; }  
    int16_t hardware_read_wing_voltage(int) { return 2400; }
    int16_t hardware_read_wing_current(int) { return 100; }
    uint16_t hardware_read_wing_power(int)  { return 24; }
    uint16_t hardware_read_wing_temp(int)   { return 38; }
};

int main(int argc, char ** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<HalServoNode>("hal_servo_node");
    rclcpp::spin(node->get_node_base_interface());
    rclcpp::shutdown();
    return 0;
}