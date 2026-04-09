#include <memory>
#include <string>
#include <chrono>
#include <cstring>  // <--- 新增这一行！
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "std_msgs/msg/byte.hpp"
#include "std_msgs/msg/string.hpp"

using namespace std::chrono_literals;
using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

class HalAntennaControlNode : public rclcpp_lifecycle::LifecycleNode {
public:
    explicit HalAntennaControlNode(const std::string & node_name)
    : rclcpp_lifecycle::LifecycleNode(node_name) {}

    // 1. 配置阶段：设置订阅者和初始变量
    CallbackReturn on_configure(const rclcpp_lifecycle::State &) override {
        RCLCPP_INFO(get_logger(), "正在配置天线控制节点 (指令序号 #15)...");
        
        step_counter_ = 0;
        is_moving_ = false;

        // 订阅指令：hal_antennacontrol_srv (来自通信管理节点)
        antenna_sub_ = this->create_subscription<std_msgs::msg::Byte>(
            "hal_antennacontrol_srv", 10,
            std::bind(&HalAntennaControlNode::antenna_callback, this, std::placeholders::_1));
        
        // 发布状态：用于观察天线位置
        status_pub_ = this->create_publisher<std_msgs::msg::String>("hal_antennacontrol_status", 10);

        return CallbackReturn::SUCCESS;
    }

    // 2. 激活阶段：启动定时器，允许电机运动
    CallbackReturn on_activate(const rclcpp_lifecycle::State & state) override {
        rclcpp_lifecycle::LifecycleNode::on_activate(state);
        RCLCPP_INFO(get_logger(), "节点已激活，等待控制指令...");
        
        // 开启 10Hz 定时器逻辑
        timer_ = this->create_wall_timer(100ms, std::bind(&HalAntennaControlNode::control_loop, this));
        return CallbackReturn::SUCCESS;
    }

    // 3. 停用阶段：强制停止电机
    CallbackReturn on_deactivate(const rclcpp_lifecycle::State & state) override {
        rclcpp_lifecycle::LifecycleNode::on_deactivate(state);
        RCLCPP_INFO(get_logger(), "节点已停用，强制停止电机。");
        stop_motor();
        is_moving_ = false;
        timer_.reset();
        return CallbackReturn::SUCCESS;
    }

private:
    // 核心循环：处理步数模拟和限位检查
    void control_loop() {
        if (!is_moving_) return;

        // 模拟限位检查（实际开发时需读取 GPIO）
        bool at_limit = (moving_dir_ == 1) ? read_limit_switch(1) : read_limit_switch(2);

        if (moving_dir_ == 1) { // 上升逻辑
            if (step_counter_ >= target_step_ || at_limit) {
                stop_motor();
                is_moving_ = false;
                RCLCPP_INFO(get_logger(), "天线已到达上限。");
            } else {
                run_motor(1);
                step_counter_++;
            }
        } 
        else if (moving_dir_ == -1) { // 下降逻辑
            if (step_counter_ <= target_step_ || at_limit) {
                stop_motor();
                is_moving_ = false;
                if(at_limit) step_counter_ = 0; // 触碰底端限位则归零
                RCLCPP_INFO(get_logger(), "天线已到达下限。");
            } else {
                run_motor(-1);
                step_counter_--;
            }
        }
    }

    // 回调函数：处理接收到的 Byte 指令
    void antenna_callback(const std_msgs::msg::Byte::SharedPtr msg) {
        if (is_moving_) {
            RCLCPP_WARN(get_logger(), "天线正在运动中，忽略新指令。");
            return;
        }

        // 0x00 上升, 0x01 下降
        if (msg->data == 0x00) {
            RCLCPP_INFO(get_logger(), "执行指令：天线上升");
            is_moving_ = true;
            moving_dir_ = 1;
            target_step_ = step_counter_ + FIXED_STEPS;
        } 
        else if (msg->data == 0x01) {
            RCLCPP_INFO(get_logger(), "执行指令：天线下降");
            is_moving_ = true;
            moving_dir_ = -1;
            target_step_ = step_counter_ - FIXED_STEPS;
        }
    }

    // 硬件底层占位函数
    void run_motor(int dir) { /* 许延轩：此处添加驱动代码 */ }
    void stop_motor() { /* 许延轩：此处添加停转代码 */ }
    bool read_limit_switch(int id) { return false; }

    rclcpp::Subscription<std_msgs::msg::Byte>::SharedPtr antenna_sub_;
    rclcpp_lifecycle::LifecyclePublisher<std_msgs::msg::String>::SharedPtr status_pub_;
    rclcpp::TimerBase::SharedPtr timer_;

    long step_counter_ = 0;
    long target_step_ = 0;
    int moving_dir_ = 0;
    bool is_moving_ = false;
    const long FIXED_STEPS = 150; // 定程步数
};

// 解决 undefined reference to main 的入口函数
int main(int argc, char * argv[])
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<HalAntennaControlNode>("hal_antennacontrol_node");
  
  // 运行节点
  rclcpp::spin(node->get_node_base_interface());
  
  rclcpp::shutdown();
  return 0;
}