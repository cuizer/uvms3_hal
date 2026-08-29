#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <cstring>

// Linux SocketCAN 底层核心库
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <unistd.h>
#include <fcntl.h>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"

// 引入您的自定义消息头文件 (修正名称以匹配 BSP 节点)

#include "hal/msg/hal_antenna.hpp"          // 状态反馈消息
#include "hal/msg/hal_antenna_control.hpp"  // 控制指令消息

using namespace std::chrono_literals;
using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

class HalAntennaControlNode : public rclcpp_lifecycle::LifecycleNode
{
public:
    explicit HalAntennaControlNode(const rclcpp::NodeOptions & options = rclcpp::NodeOptions())
    : rclcpp_lifecycle::LifecycleNode("hal_antenna_lifecycle_node", options),
      can_socket_(-1),
      can_rx_running_(false)
    {
        RCLCPP_INFO(this->get_logger(), ">>> [HAL天线大脑] 已构建，准备加载直连 CAN 驱动 <<<");
        
        // 声明参数：允许启动时通过 YAML 或命令行修改物理接口与运动限制
        this->declare_parameter<std::string>("can_interface", "can2");
        this->declare_parameter<int>("motor_tx_id", 0x141);
        this->declare_parameter<int>("speed_dps", 1800);
        this->declare_parameter<int>("lsb_down_limit", 0);
        this->declare_parameter<int>("lsb_up_limit", -12500000);
    }

    ~HalAntennaControlNode()
    {
        close_can_socket();
    }

protected:
    CallbackReturn on_configure(const rclcpp_lifecycle::State &) override
    {
        can_interface_ = this->get_parameter("can_interface").as_string();
        motor_tx_id_   = this->get_parameter("motor_tx_id").as_int();
        speed_dps_     = this->get_parameter("speed_dps").as_int();
        lsb_down_      = this->get_parameter("lsb_down_limit").as_int();
        lsb_up_        = this->get_parameter("lsb_up_limit").as_int();

        RCLCPP_INFO(this->get_logger(), "--- [配置中] 正在独占锁定网卡: %s ---", can_interface_.c_str());

        // 1. 创建 CAN 原生套接字
        can_socket_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
        if (can_socket_ < 0) {
            RCLCPP_FATAL(this->get_logger(), "❌ 创建 SocketCAN 失败！(errno: %d)", errno);
            return CallbackReturn::FAILURE;
        }

        // 2. 获取网卡索引
        struct ifreq ifr;
        strncpy(ifr.ifr_name, can_interface_.c_str(), IFNAMSIZ - 1);
        ifr.ifr_name[IFNAMSIZ - 1] = '\0';
        if (ioctl(can_socket_, SIOCGIFINDEX, &ifr) < 0) {
            RCLCPP_FATAL(this->get_logger(), "❌ 无法找到网卡 %s，请确认它是否处于 UP 状态！", can_interface_.c_str());
            close_can_socket();
            return CallbackReturn::FAILURE;
        }

        // 3. 绑定套接字到指定网卡
        struct sockaddr_can addr;
        memset(&addr, 0, sizeof(addr));
        addr.can_family = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;
        if (bind(can_socket_, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            RCLCPP_FATAL(this->get_logger(), "❌ 无法绑定套接字到 %s！", can_interface_.c_str());
            close_can_socket();
            return CallbackReturn::FAILURE;
        }

        // 4. 设置接收超时（防止读取线程死锁）
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100 毫秒超时
        setsockopt(can_socket_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

        // 5. 创建 ROS 2 订阅与发布 (修正类型与话题，与 BSP 节点完全对齐)
        auto qos = rclcpp::QoS(rclcpp::KeepLast(10)).reliable();
        cmd_sub_ = this->create_subscription<hal::msg::HalAntennaControl>(
            "/hal/antennacontrol", qos, 
            std::bind(&HalAntennaControlNode::command_callback, this, std::placeholders::_1)
        );

        state_pub_ = this->create_publisher<hal::msg::HalAntenna>("/hal/antenna", 10);

        RCLCPP_INFO(this->get_logger(), "������ [配置成功] 网卡 %s 绑定完成，系统已准备就绪。", can_interface_.c_str());
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_activate(const rclcpp_lifecycle::State & state) override
    {
        LifecycleNode::on_activate(state);
        
        if (state_pub_) { state_pub_->on_activate(); }

        // 启动后台 CAN 帧接收线程
        can_rx_running_ = true;
        rx_thread_ = std::thread(&HalAntennaControlNode::can_rx_loop, this);
        
        RCLCPP_WARN(this->get_logger(), "������������������ [节点已激活] 监听线程已拉起，随时准备收发运动指令！");
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_deactivate(const rclcpp_lifecycle::State & state) override
    {
        LifecycleNode::on_deactivate(state);
        if (state_pub_) { state_pub_->on_deactivate(); }
        
        RCLCPP_WARN(this->get_logger(), "������ [节点已钝化] 正在关闭底层监听线程...");
        stop_rx_thread();
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_cleanup(const rclcpp_lifecycle::State &) override
    {
        cmd_sub_.reset();
        state_pub_.reset();
        close_can_socket();
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_shutdown(const rclcpp_lifecycle::State &) override
    {
        stop_rx_thread();
        close_can_socket();
        return CallbackReturn::SUCCESS;
    }

private:
    void command_callback(const hal::msg::HalAntennaControl::SharedPtr msg)
    {
        if (this->get_current_state().id() != 3) {
            RCLCPP_WARN(this->get_logger(), "⚠️ 节点未处于 ACTIVE 状态，拒绝执行指令！");
            return;
        }

        uint8_t cmd_type = msg->cmd_type;
        float coeff = msg->target_coeff;
        RCLCPP_INFO(this->get_logger(), "【大脑接收指令】类型: %d, 目标系数: %.2f", cmd_type, coeff);

        // 使用硬编码数字，防止编译时找不到 msg 常量
        switch (cmd_type) {
            case 2: // CMD_UNLOCK_ONLY
                // 仅解抱闸
                send_motor_cmd(0x8C, 0x01, 0, 0);
                RCLCPP_INFO(this->get_logger(), "������ 发送解锁指令 (0x8C)");
                break;

            case 0: // CMD_UP
                execute_move(255.0f);
                break;

            case 1: // CMD_DOWN
                execute_move(0.0f);
                break;

            case 3: // CMD_CUSTOM_COEFF
                execute_move(coeff);
                break;

            case 4: // CMD_ESTOP (急停)
                // 具体的急停 CAN 指令码请参考电机手册，这里假设 0x8C, byte1=0x00 表示抱闸/急停
                send_motor_cmd(0x8C, 0x00, 0, 0); 
                RCLCPP_WARN(this->get_logger(), "������ 触发急停指令！电机已锁定");
                break;            

            case 5: // CMD_CLEAR_ESTOP
                send_motor_cmd(0x88, 0x00, 0, 0); // 清除故障
                RCLCPP_INFO(this->get_logger(), "������ 发送清除故障指令 (0x88)");
                break;

            default:
                RCLCPP_WARN(this->get_logger(), "❌ 未知指令类型");
                break;
        }
    }

    void execute_move(float coeff)
    {
        // 核心时序保护：1.清错 -> 2.等待 -> 3.解锁 -> 4.等待 -> 5.运动
        send_motor_cmd(0x88, 0x00, 0, 0); // Clear Fault
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        
        send_motor_cmd(0x8C, 0x01, 0, 0); // Release Brake
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        if (coeff >= 0.0f) {
            // 正常绝对运动模式 (0xA4)
            coeff = std::clamp(coeff, 0.0f, 255.0f);
            int32_t target_lsb = lsb_down_ + static_cast<int32_t>((coeff / 255.0f) * (lsb_up_ - lsb_down_));
            send_motor_cmd(0xA4, 0x00, speed_dps_, target_lsb);
            RCLCPP_INFO(this->get_logger(), "������ [绝对运动] 系数: %.1f -> 目标 LSB: %d", coeff, target_lsb);
        } else {
            // 异常恢复：负系数增量向下寻零模式 (0xA8)
            int32_t relative_lsb = static_cast<int32_t>((std::abs(coeff) / 255.0f) * std::abs(lsb_up_ - lsb_down_));
            // 向下运动（正数增量，具体符号看您的电机固件定义，这里假设正数为向下）
            send_motor_cmd(0xA8, 0x00, speed_dps_, relative_lsb);
            RCLCPP_WARN(this->get_logger(), "⚠️ [相对降维寻零] 系数: %.1f -> 增量 LSB: %d", coeff, relative_lsb);
        }
    }

    void send_motor_cmd(uint8_t cmd_byte, uint8_t byte1, uint16_t speed, int32_t lsb)
    {
        if (can_socket_ < 0) return;

        struct can_frame frame;
        memset(&frame, 0, sizeof(frame));
        frame.can_id = motor_tx_id_;
        frame.can_dlc = 8;

        // 小端字节序封装
        frame.data[0] = cmd_byte;
        frame.data[1] = byte1;
        frame.data[2] = speed & 0xFF;
        frame.data[3] = (speed >> 8) & 0xFF;
        frame.data[4] = lsb & 0xFF;
        frame.data[5] = (lsb >> 8) & 0xFF;
        frame.data[6] = (lsb >> 16) & 0xFF;
        frame.data[7] = (lsb >> 24) & 0xFF;

        ssize_t bytes_sent = write(can_socket_, &frame, sizeof(struct can_frame));
        if (bytes_sent != sizeof(struct can_frame)) {
            RCLCPP_ERROR(this->get_logger(), "������ [硬件拒发] 无法向 CAN 总线写入数据！可能是硬件短路或进入 Bus-Off 状态！");
        } else {
            RCLCPP_DEBUG(this->get_logger(), "[TX -> %s] ID: 0x%03X | CMD: 0x%02X", can_interface_.c_str(), frame.can_id, cmd_byte);
        }
    }

    void can_rx_loop()
    {
        struct can_frame frame;
        while (can_rx_running_) {
            ssize_t bytes_read = read(can_socket_, &frame, sizeof(struct can_frame));
            
            if (bytes_read > 0) {
                // 判断是否为电机的反馈包 (通常是 0x141 自身或 0x241，这里打印以便您分析)
                // 如果确定了 ID，可以在这里加过滤逻辑。
                // 示例：解析回传的当前位置并反向算成系数发布出去
                if (frame.can_dlc >= 8 && frame.data[0] == 0xA4) {
                    // 这是电机回传的位置反馈包
                    int32_t current_lsb = (frame.data[7] << 24) | (frame.data[6] << 16) | (frame.data[5] << 8) | frame.data[4];
                    
                    hal::msg::HalAntenna state_msg;
                    state_msg.timestamp = this->now().nanoseconds();
                    
                    // 逆向换算系数 
                    float current_coeff = 255.0f * (static_cast<float>(current_lsb - lsb_down_) / static_cast<float>(lsb_up_ - lsb_down_));
                    state_msg.total_angle = static_cast<double>(current_coeff);
                    state_msg.brake_status = 1;      // 假设运行时抱闸已解开
                    state_msg.running_status = 0;    // 状态机精简处理
                    
                    state_pub_->publish(state_msg);
                }
            } else if (bytes_read < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                RCLCPP_WARN(this->get_logger(), "CAN 接收发生硬件错误，errno: %d", errno);
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
        }
    }

    void stop_rx_thread()
    {
        can_rx_running_ = false;
        if (rx_thread_.joinable()) {
            rx_thread_.join();
        }
    }

    void close_can_socket()
    {
        if (can_socket_ >= 0) {
            close(can_socket_);
            can_socket_ = -1;
        }
    }

    // 成员变量
    std::string can_interface_;
    int motor_tx_id_;
    int speed_dps_;
    int lsb_down_;
    int lsb_up_;

    int can_socket_;
    std::atomic<bool> can_rx_running_;
    std::thread rx_thread_;

    rclcpp::Subscription<hal::msg::HalAntennaControl>::SharedPtr cmd_sub_;
    rclcpp_lifecycle::LifecyclePublisher<hal::msg::HalAntenna>::SharedPtr state_pub_;
};

int main(int argc, char ** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::NodeOptions options;
    auto node = std::make_shared<HalAntennaControlNode>(options);
    
    rclcpp::executors::SingleThreadedExecutor exe;
    exe.add_node(node->get_node_base_interface());
    exe.spin();
    
    rclcpp::shutdown();
    return 0;
}