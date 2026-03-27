#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <chrono>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "rclcpp_lifecycle/lifecycle_publisher.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "std_msgs/msg/float64_multi_array.hpp" 

// 🚨 引入我们刚刚生成的真正头文件
#include "hal/msg/hal_mainthruster_msg.hpp"
#include "hal/msg/hal_auxithruster_msg.hpp"
#include "hal/srv/hal_thrustercontrol_srv.hpp"

using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;
using std::placeholders::_1;
using std::placeholders::_2;

class HalThrusterNode : public rclcpp_lifecycle::LifecycleNode {
public:
    explicit HalThrusterNode(const std::string & node_name, bool intra_process_comms = false)
    : rclcpp_lifecycle::LifecycleNode(node_name,
        rclcpp::NodeOptions().use_intra_process_comms(intra_process_comms)) {}

    CallbackReturn on_configure(const rclcpp_lifecycle::State &) override {
        RCLCPP_INFO(get_logger(), "配置中... 初始化推进器节点接口。");

        // 🚨 命名空间全部改为 hal
        pub_main_status_ = this->create_publisher<hal::msg::HalMainthrusterMsg>(
            "hal_mainthruster_msg", 10);
            
        pub_aux_status_ = this->create_publisher<hal::msg::HalAuxithrusterMsg>(
            "hal_auxithruster_msg", 10);

        srv_control_ = this->create_service<hal::srv::HalThrustercontrolSrv>(
            "hal_thrustercontrol_srv",
            std::bind(&HalThrusterNode::control_srv_callback, this, _1, _2));

        sub_cmd_ = this->create_subscription<std_msgs::msg::Float64MultiArray>(
            "/hal/thruster/cmd", 10,
            std::bind(&HalThrusterNode::cmd_callback, this, _1));

        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(20),
            std::bind(&HalThrusterNode::timer_publish_status_callback, this));

        hardware_api_init_can(500000);
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_activate(const rclcpp_lifecycle::State & state) override {
        RCLCPP_INFO(get_logger(), "推进器节点已激活。");
        pub_main_status_->on_activate();
        pub_aux_status_->on_activate();
        is_testing_ = false;
        LifecycleNode::on_activate(state);
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_deactivate(const rclcpp_lifecycle::State & state) override {
        pub_main_status_->on_deactivate();
        pub_aux_status_->on_deactivate();
        stop_all_thrusters();
        LifecycleNode::on_deactivate(state);
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_cleanup(const rclcpp_lifecycle::State &) override {
        pub_main_status_.reset(); pub_aux_status_.reset();
        srv_control_.reset(); sub_cmd_.reset(); timer_.reset();
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_shutdown(const rclcpp_lifecycle::State &) override {
        stop_all_thrusters();
        return CallbackReturn::SUCCESS;
    }

private:
    bool is_testing_ = false; 

    std::shared_ptr<rclcpp_lifecycle::LifecyclePublisher<hal::msg::HalMainthrusterMsg>> pub_main_status_;
    std::shared_ptr<rclcpp_lifecycle::LifecyclePublisher<hal::msg::HalAuxithrusterMsg>> pub_aux_status_;
    rclcpp::Service<hal::srv::HalThrustercontrolSrv>::SharedPtr srv_control_;
    rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr sub_cmd_;
    rclcpp::TimerBase::SharedPtr timer_;

    void control_srv_callback(
        const std::shared_ptr<hal::srv::HalThrustercontrolSrv::Request> request,
        std::shared_ptr<hal::srv::HalThrustercontrolSrv::Response> response) 
    {
        if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) {
            response->success = false; response->message = "节点未激活，无法执行测试。"; return;
        }

        if (is_testing_) {
            response->success = false; response->message = "推进器正在测试中，请稍后再试。"; return;
        }

        uint8_t cmd = request->command;
        if (cmd == 0x01) {
            RCLCPP_INFO(get_logger(), "收到指令 01：开始【主推】正反转各3秒测试...");
            std::thread(&HalThrusterNode::execute_test_sequence, this, true).detach();
            response->success = true; response->message = "主推测试序列已启动";
        } 
        else if (cmd == 0x02) {
            RCLCPP_INFO(get_logger(), "收到指令 02：开始【辅推】正反转各3秒测试...");
            std::thread(&HalThrusterNode::execute_test_sequence, this, false).detach();
            response->success = true; response->message = "辅推测试序列已启动";
        } 
        else {
            response->success = false; response->message = "未知指令代码";
        }
    }

    void execute_test_sequence(bool is_main_thruster) {
        is_testing_ = true; 
        double test_rpm = 3000.0 * 0.10; 
        
        set_thruster_rpm_hardware(is_main_thruster, test_rpm);
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        set_thruster_rpm_hardware(is_main_thruster, -test_rpm);
        std::this_thread::sleep_for(std::chrono::seconds(3));

        set_thruster_rpm_hardware(is_main_thruster, 0.0);
        is_testing_ = false; 
        RCLCPP_INFO(get_logger(), "测试序列执行完毕，恢复正常控制。");
    }

    void cmd_callback(const std_msgs::msg::Float64MultiArray::SharedPtr msg) {
        if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) return;
        if (is_testing_) return; 

        if (msg->data.size() >= 7) {
            set_thruster_rpm_hardware(true, msg->data[0]);  
            for (int i = 1; i < 7; ++i) {
                // 驱动辅推底层接口
            }
        }
    }

    void timer_publish_status_callback() {
        if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) return;

        hal::msg::HalMainthrusterMsg main_msg;
        main_msg.rpm = hardware_read_main_rpm();         
        main_msg.current = hardware_read_main_current(); 
        main_msg.voltage = hardware_read_main_voltage(); 
        main_msg.fault_status = 0x00;                    
        pub_main_status_->publish(main_msg);

        hal::msg::HalAuxithrusterMsg aux_msg;
        for (int i = 0; i < 6; ++i) {
            aux_msg.rpm[i] = hardware_read_aux_rpm(i);         
            aux_msg.current[i] = hardware_read_aux_current(i); 
            aux_msg.voltage[i] = hardware_read_aux_voltage(i); 
            aux_msg.temp[i] = hardware_read_aux_temp(i);       
            aux_msg.esc_status[i] = 0x01;                      
            aux_msg.fault_status[i] = 0x00;                    
        }
        pub_aux_status_->publish(aux_msg);
    }

    // --- 底层硬件接口占位 ---
    void hardware_api_init_can(int) {}
    void stop_all_thrusters() {}
    void set_thruster_rpm_hardware(bool /*is_main*/, double /*rpm*/) {}
    
    int16_t hardware_read_main_rpm() { return 1500; }
    int16_t hardware_read_main_current() { return 500; }
    int16_t hardware_read_main_voltage() { return 4800; }
    
    int16_t hardware_read_aux_rpm(int) { return 1000; }
    int16_t hardware_read_aux_current(int) { return 200; }
    int16_t hardware_read_aux_voltage(int) { return 2400; }
    uint16_t hardware_read_aux_temp(int) { return 45; }
};

int main(int argc, char ** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<HalThrusterNode>("hal_thruster_node");
    rclcpp::spin(node->get_node_base_interface());
    rclcpp::shutdown();
    return 0;
}