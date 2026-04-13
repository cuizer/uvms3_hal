#include <memory>
#include <string>
#include <chrono>
#include <cstring>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "std_msgs/msg/u_int8.hpp"

#include "hal/msg/hal_antenna.hpp"
#include "hal/msg/can_msg_in.hpp"
#include "hal/msg/can_msg_out.hpp"

using namespace std::chrono_literals;
using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

class HalAntennaControlNode : public rclcpp_lifecycle::LifecycleNode {
public:
    explicit HalAntennaControlNode(const std::string & node_name)
    : rclcpp_lifecycle::LifecycleNode(node_name) {}

    CallbackReturn on_configure(const rclcpp_lifecycle::State &) override {
        RCLCPP_INFO(get_logger(), "配置天线节点。模式：自动状态监测 + 手动逻辑控制。");
        is_moving_ = false;
        
        // 订阅与发布初始化
        antenna_sub_ = this->create_subscription<std_msgs::msg::UInt8>(
            "hal_antennacontrol_srv", 10,
            std::bind(&HalAntennaControlNode::antenna_callback, this, std::placeholders::_1));
        
        canin_sub_ = this->create_subscription<hal::msg::CanMsgIn>(
            "/hal/canin", 10, 
            std::bind(&HalAntennaControlNode::canin_callback, this, std::placeholders::_1));

        status_pub_ = this->create_publisher<hal::msg::HalAntenna>("/hal/antenna", 10);
        canout_pub_ = this->create_publisher<hal::msg::CanMsgOut>("/hal/canout", 10);

        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_activate(const rclcpp_lifecycle::State & state) override {
        rclcpp_lifecycle::LifecycleNode::on_activate(state);
        status_pub_->on_activate();
        canout_pub_->on_activate();

        // 激活即开启定时器：负责状态轮询和限位检查
        RCLCPP_INFO(get_logger(), "节点已激活，开始自动轮询硬件状态...");
        timer_ = this->create_wall_timer(100ms, std::bind(&HalAntennaControlNode::control_loop, this));
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_deactivate(const rclcpp_lifecycle::State & state) override {
        rclcpp_lifecycle::LifecycleNode::on_deactivate(state);
        stop_all(); // 停用时安全关闭
        status_pub_->on_deactivate();
        canout_pub_->on_deactivate();
        timer_.reset();
        return CallbackReturn::SUCCESS;
    }

private:
    // 物理参数
    const double POS_DOWN_LIMIT = 0.0;         
    const double POS_UP_LIMIT = -125000.0;     
    const uint16_t MOTOR_RUN_SPEED = 8000;     

    // 运行状态变量
    double current_total_angle_ = 0.0; 
    uint8_t current_run_status_ = 0;
    uint8_t current_brake_status_ = 0xFF; // 初始设为未知
    uint8_t query_cnt_ = 0; 
    int moving_dir_ = 0;
    bool is_moving_ = false;

    // 手动指令处理
    void antenna_callback(const std_msgs::msg::UInt8::SharedPtr msg) {
        switch (msg->data) {
            case 0: // 上升
                if (current_brake_status_ == 0x01) {
                    moving_dir_ = -1;
                    run_motor(static_cast<int32_t>(POS_UP_LIMIT * 100));
                    RCLCPP_INFO(get_logger(), "指令：执行上升动作");
                } else {
                    RCLCPP_ERROR(get_logger(), "拒绝动作：抱闸尚未解锁！请先发送 2 解锁。");
                }
                break;
            case 1: // 下降
                if (current_brake_status_ == 0x01) {
                    moving_dir_ = 1;
                    run_motor(static_cast<int32_t>(POS_DOWN_LIMIT * 100));
                    RCLCPP_INFO(get_logger(), "指令：执行下降动作");
                } else {
                    RCLCPP_ERROR(get_logger(), "拒绝动作：抱闸尚未解锁！请先发送 2 解锁。");
                }
                break;
            case 2: // 手动解锁
                RCLCPP_INFO(get_logger(), "指令：发送解锁指令 (0x8C 01)");
                send_can_cmd(0x8C, 0x01);
                break;
            case 3: // 手动加锁
                RCLCPP_INFO(get_logger(), "指令：发送加锁指令 (0x8C 00)");
                send_can_cmd(0x8C, 0x00);
                break;
            case 4: // 停止电机
                RCLCPP_INFO(get_logger(), "指令：停止旋转");
                stop_motor_only();
                break;
        }
    }

    // 定时器：自动检查状态 + 限位保护
    void control_loop() {
        // 1. 限位安全检查
        if (is_moving_) {
            bool hit_up = (moving_dir_ == -1 && current_total_angle_ <= POS_UP_LIMIT);
            bool hit_down = (moving_dir_ == 1 && current_total_angle_ >= POS_DOWN_LIMIT);
            if (hit_up || hit_down) {
                RCLCPP_WARN(get_logger(), "软限位触发，自动停止旋转。");
                stop_motor_only();
            }
        }

        // 2. 自动状态轮询 (分步查询以防总线拥堵)
        if (query_cnt_ == 0)      { send_can_cmd(0x92, 0x00); query_cnt_ = 1; } // 查询角度
        else if (query_cnt_ == 1) { send_can_cmd(0x9C, 0x00); query_cnt_ = 2; } // 查询速度/状态
        else                      { send_can_cmd(0x8C, 0x10); query_cnt_ = 0; } // 查询抱闸状态
    }

    // CAN 回传解析
    void canin_callback(const hal::msg::CanMsgIn::SharedPtr msg) {
        if (msg->id == 0x184) {
            if (msg->data[0] == 0x92) { // 角度解析
                uint64_t raw = 0;
                for(int i = 0; i < 7; ++i) raw |= (static_cast<uint64_t>(msg->data[i+1]) << (8 * i));
                if (raw & (1ULL << 55)) raw |= 0xFF00000000000000ULL;
                current_total_angle_ = static_cast<double>(static_cast<int64_t>(raw)) / 100.0;
            }
            else if (msg->data[0] == 0x9C) { // 速度解析
                int16_t speed = msg->data[4] | (msg->data[5] << 8);
                current_run_status_ = (speed == 0) ? 0x00 : 0x01;
                if (current_run_status_ == 0x00) is_moving_ = false;
            }
            else if (msg->data[0] == 0x8C) { // 抱闸解析
                current_brake_status_ = msg->data[1]; 
            }

            // 发布到 ROS 2 供上位机/你观察
            hal::msg::HalAntenna s_msg;
            s_msg.brake_status = current_brake_status_;
            s_msg.run_status   = current_run_status_;
            s_msg.total_angle  = current_total_angle_;
            status_pub_->publish(s_msg);
        }
    }

    // 辅助函数
    void send_can_cmd(uint8_t b0, uint8_t b1, uint8_t b2=0, uint8_t b3=0, uint8_t b4=0, uint8_t b5=0, uint8_t b6=0, uint8_t b7=0) {
        hal::msg::CanMsgOut can_msg;
        can_msg.id = 0x144; can_msg.dlc = 8;
        can_msg.data[0]=b0; can_msg.data[1]=b1; can_msg.data[2]=b2; can_msg.data[3]=b3;
        can_msg.data[4]=b4; can_msg.data[5]=b5; can_msg.data[6]=b6; can_msg.data[7]=b7;
        canout_pub_->publish(can_msg);
    }

    void run_motor(int32_t target) { 
        send_can_cmd(0xA4, 0x00, MOTOR_RUN_SPEED & 0xFF, (MOTOR_RUN_SPEED >> 8) & 0xFF, 
                     target & 0xFF, (target >> 8) & 0xFF, (target >> 16) & 0xFF, (target >> 24) & 0xFF);
        is_moving_ = true; 
    }
    
    void stop_motor_only() { send_can_cmd(0x81, 0x00); is_moving_ = false; }
    void stop_all() { send_can_cmd(0x81, 0x00); send_can_cmd(0x8C, 0x00); is_moving_ = false; }

    // 成员变量
    rclcpp::Subscription<std_msgs::msg::UInt8>::SharedPtr antenna_sub_;
    rclcpp::Subscription<hal::msg::CanMsgIn>::SharedPtr canin_sub_;
    rclcpp_lifecycle::LifecyclePublisher<hal::msg::HalAntenna>::SharedPtr status_pub_;
    rclcpp_lifecycle::LifecyclePublisher<hal::msg::CanMsgOut>::SharedPtr canout_pub_;
    rclcpp::TimerBase::SharedPtr timer_;
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<HalAntennaControlNode>("hal_antennacontrol_node");
    rclcpp::spin(node->get_node_base_interface());
    rclcpp::shutdown();
    return 0;
}
