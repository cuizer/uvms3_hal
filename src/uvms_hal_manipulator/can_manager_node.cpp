#include "rclcpp/rclcpp.hpp"
#include "hal/msg/can_frame_manipulator.hpp"
#include "uvms_hal_manipulator/can_driver.hpp"

#include <deque>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <map>
#include <string>
#include <memory>
#include <stdexcept>
#include <cstdint>

namespace uvms_hal_manipulator
{

class CanManagerNode : public rclcpp::Node
{
public:
    CanManagerNode()
    : Node("can_manager"),
      send_interval_us_(300),
      max_queue_size_(500),
      running_(true)
    {
        // ========================================================
        // ROS2 参数注意：
        // 不要用 size_t 作为 declare_parameter 类型。
        // Humble 中 size_t / unsigned long 会导致 ParameterValue 构造歧义。
        // ========================================================
        this->declare_parameter<std::string>("can_interface", "can4");
        this->declare_parameter<int>("send_interval_us", 300);
        this->declare_parameter<int>("max_queue_size", 500);

        can_interface_ = this->get_parameter("can_interface").as_string();
        send_interval_us_ = static_cast<int>(
            this->get_parameter("send_interval_us").as_int());

        int max_queue_size_param = static_cast<int>(
            this->get_parameter("max_queue_size").as_int());

        if (send_interval_us_ <= 0) {
            RCLCPP_WARN(
                this->get_logger(),
                "send_interval_us <= 0, reset to 300 us.");
            send_interval_us_ = 300;
        }

        if (max_queue_size_param <= 0) {
            RCLCPP_WARN(
                this->get_logger(),
                "max_queue_size <= 0, reset to 500.");
            max_queue_size_param = 500;
        }

        max_queue_size_ = static_cast<size_t>(max_queue_size_param);

        // ========================================================
        // /hal/can_rx：can_manager 从 can0 读到底层 CAN 帧后，
        // 转换成 ROS 消息发布出去。
        // ========================================================
        can_rx_pub_ = this->create_publisher<hal::msg::CanFrameManipulator>(
            "/hal/can_rx",
            rclcpp::QoS(200));

        // ========================================================
        // /hal/can_tx：其他节点只发布 CAN 请求，
        // can_manager 统一排队、限频并写入 can0。
        // ========================================================
        can_tx_sub_ = this->create_subscription<hal::msg::CanFrameManipulator>(
            "/hal/can_tx",
            rclcpp::QoS(200),
            [this](hal::msg::CanFrameManipulator::SharedPtr msg)
            {
                this->can_tx_callback(msg);
            });

        if (!can_driver_.open(can_interface_)) {
            RCLCPP_FATAL(
                this->get_logger(),
                "Failed to open CAN interface: %s",
                can_interface_.c_str());
            throw std::runtime_error("Failed to open CAN interface");
        }

        can_driver_.flush();

        tx_thread_ = std::thread(&CanManagerNode::tx_loop, this);
        rx_thread_ = std::thread(&CanManagerNode::rx_loop, this);

        RCLCPP_INFO(
            this->get_logger(),
            "CAN manager started. interface=%s, send_interval_us=%d, max_queue_size=%zu",
            can_interface_.c_str(),
            send_interval_us_,
            max_queue_size_);
    }

    ~CanManagerNode() override
    {
        running_ = false;

        if (tx_thread_.joinable()) {
            tx_thread_.join();
        }

        if (rx_thread_.joinable()) {
            rx_thread_.join();
        }

        can_driver_.close();

        RCLCPP_INFO(
            this->get_logger(),
            "CAN manager stopped.");
    }

private:
    void can_tx_callback(const hal::msg::CanFrameManipulator::SharedPtr msg)
    {
        if (!msg) {
            return;
        }

        if (msg->dlc > 8) {
            RCLCPP_WARN(
                this->get_logger(),
                "Reject CAN frame with invalid dlc=%u, can_id=%u",
                static_cast<unsigned int>(msg->dlc),
                msg->can_id);
            return;
        }

        std::lock_guard<std::mutex> lock(queue_mutex_);

        hal::msg::CanFrameManipulator frame = *msg;
        frame.stamp = this->now();

        // ========================================================
        // 控制帧处理：
        // 对同一个 can_id 的 0x44，只保留最新一帧。
        // 这样可以避免旧位置目标排队太久后再被发送，导致动作滞后。
        // ========================================================
        if (is_control_frame(frame)) {
            latest_control_frames_[frame.can_id] = frame;
            return;
        }

        // ========================================================
        // 查询帧处理：
        // 查询类帧进入普通队列。
        // 如果队列过长，丢弃最旧查询帧，避免占满总线。
        // ========================================================
        if (normal_queue_.size() >= max_queue_size_) {
            normal_queue_.pop_front();

            RCLCPP_WARN_THROTTLE(
                this->get_logger(),
                *this->get_clock(),
                2000,
                "CAN tx queue full. Drop oldest normal frame.");
        }

        normal_queue_.push_back(frame);
    }

    bool is_control_frame(const hal::msg::CanFrameManipulator& frame) const
    {
        if (frame.dlc >= 1 && frame.data[0] == 0x44) {
            return true;
        }

        if (frame.frame_type == "control") {
            return true;
        }

        return false;
    }

    bool pop_next_frame(hal::msg::CanFrameManipulator& out)
    {
        std::lock_guard<std::mutex> lock(queue_mutex_);

        // ========================================================
        // 1. 优先发送最新控制帧。
        // latest_control_frames_ 里每个 can_id 只保留最新目标。
        // ========================================================
        if (!latest_control_frames_.empty()) {
            auto it = latest_control_frames_.begin();
            out = it->second;
            latest_control_frames_.erase(it);
            return true;
        }

        // ========================================================
        // 2. 再发送普通查询帧。
        // ========================================================
        if (!normal_queue_.empty()) {
            out = normal_queue_.front();
            normal_queue_.pop_front();
            return true;
        }

        return false;
    }

    void tx_loop()
    {
        while (running_) {
            hal::msg::CanFrameManipulator msg;

            if (!pop_next_frame(msg)) {
                std::this_thread::sleep_for(std::chrono::microseconds(200));
                continue;
            }

            // ====================================================
            // ROS 消息 -> 底层 CAN 帧
            //
            // 注意：
            // can_driver_.write_frame() 接收的是底层 CanFrame，
            // 不是 hal::msg::CanFrameManipulator。
            // ====================================================
            CanFrame tx;
            tx.can_id = msg.can_id;
            tx.dlc = msg.dlc;

            for (size_t i = 0; i < 8; ++i) {
                tx.data[i] = msg.data[i];
            }

            bool ok = can_driver_.write_frame(tx);

            if (!ok) {
                RCLCPP_WARN_THROTTLE(
                    this->get_logger(),
                    *this->get_clock(),
                    2000,
                    "Failed to write CAN frame. can_id=%u, cmd=0x%02X, source=%s, frame_type=%s",
                    msg.can_id,
                    msg.dlc > 0 ? msg.data[0] : 0,
                    msg.source.c_str(),
                    msg.frame_type.c_str());
            }

            std::this_thread::sleep_for(
                std::chrono::microseconds(send_interval_us_));
        }
    }

    void rx_loop()
    {
        while (running_) {
            // ====================================================
            // 底层 CAN 帧
            //
            // 注意：
            // can_driver_.read_frame() 读出来的是底层 CanFrame。
            // 然后再转换成 hal::msg::CanFrameManipulator 发布到 /hal/can_rx。
            // ====================================================
            CanFrame rx;

            if (!can_driver_.read_frame(rx)) {
                std::this_thread::sleep_for(std::chrono::microseconds(200));
                continue;
            }

            if (rx.dlc > 8) {
                RCLCPP_WARN_THROTTLE(
                    this->get_logger(),
                    *this->get_clock(),
                    2000,
                    "Drop invalid CAN rx frame. can_id=%u, dlc=%u",
                    rx.can_id,
                    static_cast<unsigned int>(rx.dlc));
                continue;
            }

            hal::msg::CanFrameManipulator msg;
            msg.stamp = this->now();
            msg.can_id = rx.can_id;
            msg.dlc = rx.dlc;
            msg.priority = 0;
            msg.source = "can_manager";
            msg.frame_type = "rx_feedback";

            for (size_t i = 0; i < 8; ++i) {
                msg.data[i] = rx.data[i];
            }

            can_rx_pub_->publish(msg);
        }
    }

private:
    std::string can_interface_;
    int send_interval_us_;
    size_t max_queue_size_;

    CanDriver can_driver_;

    rclcpp::Subscription<hal::msg::CanFrameManipulator>::SharedPtr can_tx_sub_;
    rclcpp::Publisher<hal::msg::CanFrameManipulator>::SharedPtr can_rx_pub_;

    std::thread tx_thread_;
    std::thread rx_thread_;
    std::atomic<bool> running_;

    std::mutex queue_mutex_;

    // 普通查询帧队列，例如 0x08、0x0A、0x31
    std::deque<hal::msg::CanFrameManipulator> normal_queue_;

    // 控制帧缓存：每个 can_id 只保留最新 0x44
    std::map<uint32_t, hal::msg::CanFrameManipulator> latest_control_frames_;
};

}  // namespace uvms_hal_manipulator

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);

    auto node = std::make_shared<uvms_hal_manipulator::CanManagerNode>();

    rclcpp::spin(node);

    rclcpp::shutdown();

    return 0;
}