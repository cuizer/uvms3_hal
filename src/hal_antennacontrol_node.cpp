#include <memory>
#include <string>
#include <cstring>
#include <vector>
#include <algorithm>
#include <iostream>
#include <thread>
#include <chrono>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "std_msgs/msg/u_int8.hpp"
#include "std_msgs/msg/float32.hpp" // 【新增】用于接收自定义角度输入

#include "hal/msg/hal_antenna.hpp"
#include "hal/msg/can_msg_in.hpp"
#include "hal/msg/can_msg_out.hpp"

using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

class HalAntennaControlNode : public rclcpp_lifecycle::LifecycleNode {
public:
    explicit HalAntennaControlNode(const std::string & node_name)
    : rclcpp_lifecycle::LifecycleNode(node_name) {}

    CallbackReturn on_configure(const rclcpp_lifecycle::State &) override {
        RCLCPP_INFO(this->get_logger(), "[系统] 配置完成：天线控制复合模式 (电机ID: 0x141)");

        // 标准安全控制主题订阅 (0-6指令)
        antenna_sub_ = this->create_subscription<std_msgs::msg::UInt8>(
            "/hal_antennacontrol_srv", 10,
            std::bind(&HalAntennaControlNode::antenna_callback, this, std::placeholders::_1));

        // 【新增】自定义越权角度控制主题订阅 (无视限位)
        override_angle_sub_ = this->create_subscription<std_msgs::msg::Float32>(
            "/hal_antennacontrol_override_angle", 10,
            std::bind(&HalAntennaControlNode::override_angle_callback, this, std::placeholders::_1));

        // CAN 总线接收主题订阅
        canin_sub_ = this->create_subscription<hal::msg::CanMsgIn>(
            "/hal/canin", 10,
            std::bind(&HalAntennaControlNode::canin_callback, this, std::placeholders::_1));

        // 发布主题注册
        status_pub_ = this->create_publisher<hal::msg::HalAntenna>("/hal/antenna", 10);
        canout_pub_ = this->create_publisher<hal::msg::CanMsgOut>("/hal/canout", 10);

        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_activate(const rclcpp_lifecycle::State & state) override {
        rclcpp_lifecycle::LifecycleNode::on_activate(state);
        status_pub_->on_activate();
        canout_pub_->on_activate();
        RCLCPP_INFO(this->get_logger(), "[系统] 节点已激活。准备接收指令...");
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_deactivate(const rclcpp_lifecycle::State & state) override {
        send_can_cmd(0x81, 0x00); // 停止电机
        status_pub_->on_deactivate();
        canout_pub_->on_deactivate();
        rclcpp_lifecycle::LifecycleNode::on_deactivate(state);
        RCLCPP_INFO(this->get_logger(), "[系统] 节点已失活。");
        return CallbackReturn::SUCCESS;
    }

private:
    // --- 核心参数配置 ---
    const uint16_t MOTOR_RUN_SPEED = 1800;    // 速度限制：1800 dps
    const int32_t TARGET_UP = -12500000;      // 上升目标 LSB (-1250.00度)
    const int32_t TARGET_DOWN = 0;            // 下降目标 LSB (0.00度)

    // --- 标准安全指令回调 ---
    void antenna_callback(const std_msgs::msg::UInt8::SharedPtr msg) {
        RCLCPP_INFO(this->get_logger(), "\n[操作] 收到请求编号: %d", (int)msg->data);
        switch (msg->data) {
            case 0: // 复合上升序列
                RCLCPP_INFO(this->get_logger(), ">> 启动上升序列：解锁 -> 使能 -> 运动至 %d", TARGET_UP);
                send_can_cmd(0x8C, 0x01); // 1. 释放抱闸
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                send_can_cmd(0x88, 0x00); // 2. 电机使能
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                run_motor(TARGET_UP);     // 3. 位置控制
                break;

            case 1: // 复合下降序列
                RCLCPP_INFO(this->get_logger(), ">> 启动下降序列：解锁 -> 使能 -> 运动至 %d", TARGET_DOWN);
                send_can_cmd(0x8C, 0x01);
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                send_can_cmd(0x88, 0x00);
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                run_motor(TARGET_DOWN);
                break;

            case 2: send_can_cmd(0x8C, 0x01); break; // 仅手动解锁
            case 3: send_can_cmd(0x8C, 0x00); break; // 仅手动锁定
            case 4: send_can_cmd(0x81, 0x00); break; // 仅紧急停止
            case 5: send_can_cmd(0x8C, 0x10); break; // 仅查询抱闸
            case 6: send_can_cmd(0x92, 0x00); break; // 仅查询位置
            default: RCLCPP_WARN(this->get_logger(), "[警告] 未知指令编号!"); break;
        }
    }

    // --- 【新增】无限制自定义角度回调 ---
    void override_angle_callback(const std_msgs::msg::Float32::SharedPtr msg) {
        double target_degree = msg->data;
        // 角度转 LSB (比例基于代码注释: -12500000 LSB = -1250.00度)
        int32_t target_lsb = static_cast<int32_t>(target_degree * 10000.0);

        RCLCPP_WARN(this->get_logger(), "\n[越权操作] 收到自定义角度: %.2f 度 (LSB: %d)", target_degree, target_lsb);
        RCLCPP_INFO(this->get_logger(), ">> 正在无视软件限位执行：解锁 -> 使能 -> 强制运动");

        send_can_cmd(0x8C, 0x01); // 1. 释放抱闸
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        send_can_cmd(0x88, 0x00); // 2. 电机使能
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        run_motor(target_lsb);    // 3. 强制位置控制
    }

    // --- CAN 反馈解析回调 ---
    void canin_callback(const hal::msg::CanMsgIn::SharedPtr msg) {
        // 【已修复】同时兼容 0x141 和 0x181 作为反馈 ID
        if (msg->id == 0x141 || msg->id == 0x181) {

            // 解析多圈角度反馈 (0x92)
            if (msg->data[0] == 0x92) {
                uint64_t raw = 0;
                for (int i = 0; i < 7; ++i) {
                    raw |= (static_cast<uint64_t>(msg->data[i + 1]) << (8 * i));
                }
                // 56位有符号数符号扩展
                if (raw & (1ULL << 55)) raw |= 0xFF00000000000000ULL;

                int64_t motorAngle = static_cast<int64_t>(raw);
                double degree = motorAngle / 10000.0; // 统一为1度=10000LSB的解析逻辑

                RCLCPP_INFO(this->get_logger(), "[反馈] 当前多圈位置: %.2f 度", degree);
            }
            // 解析抱闸状态反馈 (0x8C)
            else if (msg->data[0] == 0x8C) {
                uint8_t brake = msg->data[1];
                std::string brake_status_str = (brake == 0x01) ? "已释放(通电)" : "已锁定(断电)";
                RCLCPP_INFO(this->get_logger(), "[反馈] 抱闸状态: %s", brake_status_str.c_str());

                hal::msg::HalAntenna s_msg;
                s_msg.brake_status = brake;
                status_pub_->publish(s_msg);
            }
        }
    }

    // --- 发送基础 CAN 指令 ---
    void send_can_cmd(uint8_t b0, uint8_t b1) {
        hal::msg::CanMsgOut msg;
        msg.id = 0x141; // ID 141
        msg.dlc = 8;
        std::fill(msg.data.begin(), msg.data.end(), 0x00);
        msg.data[0] = b0; msg.data[1] = b1;
        canout_pub_->publish(msg);
        RCLCPP_INFO(this->get_logger(), "[CAN发送] ID: 0x141 | 指令: 0x%02X 参数: 0x%02X", b0, b1);
    }

    // --- 发送 A4 多圈位置控制指令 ---
    void run_motor(int32_t target) {
        hal::msg::CanMsgOut msg;
        msg.id = 0x141; msg.dlc = 8;
        std::fill(msg.data.begin(), msg.data.end(), 0x00);
        msg.data[0] = 0xA4;
        msg.data[2] = MOTOR_RUN_SPEED & 0xFF; // 速度限制低字节
        msg.data[3] = (MOTOR_RUN_SPEED >> 8) & 0xFF; // 速度限制高字节
        msg.data[4] = target & 0xFF;
        msg.data[5] = (target >> 8) & 0xFF;
        msg.data[6] = (target >> 16) & 0xFF;
        msg.data[7] = (target >> 24) & 0xFF;
        canout_pub_->publish(msg);
        RCLCPP_INFO(this->get_logger(), "[CAN发送] ID: 0x141 | 目标位置: %d LSB", target);
    }

    // --- 成员变量定义 ---
    rclcpp::Subscription<std_msgs::msg::UInt8>::SharedPtr antenna_sub_;
    rclcpp::Subscription<std_msgs::msg::Float32>::SharedPtr override_angle_sub_; // 【新增】
    rclcpp::Subscription<hal::msg::CanMsgIn>::SharedPtr canin_sub_;

    rclcpp_lifecycle::LifecyclePublisher<hal::msg::HalAntenna>::SharedPtr status_pub_;
    rclcpp_lifecycle::LifecyclePublisher<hal::msg::CanMsgOut>::SharedPtr canout_pub_;
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<HalAntennaControlNode>("hal_antennacontrol_node");
    rclcpp::spin(node->get_node_base_interface());
    rclcpp::shutdown();
    return 0;
}
