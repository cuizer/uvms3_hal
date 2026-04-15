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

#include "hal/msg/hal_antenna.hpp"
#include "hal/msg/can_msg_in.hpp"
#include "hal/msg/can_msg_out.hpp"

using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

class HalAntennaControlNode : public rclcpp_lifecycle::LifecycleNode {
public:
    explicit HalAntennaControlNode(const std::string & node_name)
    : rclcpp_lifecycle::LifecycleNode(node_name) {}

    CallbackReturn on_configure(const rclcpp_lifecycle::State &) override {
        std::cout << "[系统] 配置完成：天线控制复合模式 (电机ID: 0x141)" << std::endl;

        antenna_sub_ = this->create_subscription<std_msgs::msg::UInt8>(
            "/hal_antennacontrol_srv", 10,
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
        std::cout << "[系统] 节点已激活。指令触发将自动执行 [解锁+使能+运动] 序列。" << std::endl;
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_deactivate(const rclcpp_lifecycle::State & state) override {
        send_can_cmd(0x81, 0x00); // 停止电机
        status_pub_->on_deactivate();
        canout_pub_->on_deactivate();
        rclcpp_lifecycle::LifecycleNode::on_deactivate(state);
        return CallbackReturn::SUCCESS;
    }

private:
    // --- 核心参数配置 ---
    const uint16_t MOTOR_RUN_SPEED = 1800;    // 速度限制：1800 dps
    const int32_t TARGET_UP = -12500000;     // 上升目标 LSB (-1250.00度)
    const int32_t TARGET_DOWN = 0;           // 下降目标 LSB (0.00度)

    void antenna_callback(const std_msgs::msg::UInt8::SharedPtr msg) {
        std::cout << "\n[操作] 收到请求编号: " << (int)msg->data << std::endl;
        switch (msg->data) {
            case 0: // 复合上升序列
                std::cout << ">> 启动上升序列：解锁 -> 使能 -> 运动至 " << TARGET_UP << std::endl;
                send_can_cmd(0x8C, 0x01); // 1. 释放抱闸
                std::this_thread::sleep_for(std::chrono::milliseconds(200));
                send_can_cmd(0x88, 0x00); // 2. 电机使能
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                run_motor(TARGET_UP);     // 3. 位置控制
                break;

            case 1: // 复合下降序列
                std::cout << ">> 启动下降序列：解锁 -> 使能 -> 运动至 " << TARGET_DOWN << std::endl;
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
        }
    }

    void canin_callback(const hal::msg::CanMsgIn::SharedPtr msg) {
        // 解析 1 号电机反馈 (0x180 + ID1 = 0x181)
        if (msg->id == 0x181) {
            // 解析多圈角度反馈 (0x92)
            if (msg->data[0] == 0x92) {
                uint64_t raw = 0;
                for (int i = 0; i < 7; ++i) {
                    raw |= (static_cast<uint64_t>(msg->data[i + 1]) << (8 * i));
                }
                // 56位有符号数符号扩展
                if (raw & (1ULL << 55)) raw |= 0xFF00000000000000ULL;
                int64_t motorAngle = static_cast<int64_t>(raw);
                double degree = motorAngle / 100.0;
                std::cout << "[反馈] 当前多圈位置: " << degree << " 度" << std::endl;
            }
            // 解析抱闸状态反馈 (0x8C)
            else if (msg->data[0] == 0x8C) {
                uint8_t brake = msg->data[1];
                std::cout << "[反馈] 抱闸状态: " << (brake == 0x01 ? "已释放(通电)" : "已锁定(断电)") << std::endl;

                hal::msg::HalAntenna s_msg;
                s_msg.brake_status = brake;
                status_pub_->publish(s_msg);
            }
        }
    }

    // 发送基础 CAN 指令
    void send_can_cmd(uint8_t b0, uint8_t b1) {
        hal::msg::CanMsgOut msg;
        msg.id = 0x141; // ID 141
        msg.dlc = 8;
        std::fill(msg.data.begin(), msg.data.end(), 0x00);
        msg.data[0] = b0; msg.data[1] = b1;
        canout_pub_->publish(msg);
        std::cout << "[CAN发送] ID: 0x141 | 指令: 0x" << std::hex << (int)b0 << " 参数: 0x" << (int)b1 << std::dec << std::endl;
    }

    // 发送 A4 多圈位置控制指令
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
        std::cout << "[CAN发送] ID: 0x141 | 目标位置: " << target << " LSB" << std::endl;
    }

    rclcpp::Subscription<std_msgs::msg::UInt8>::SharedPtr antenna_sub_;
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

