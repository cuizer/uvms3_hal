#include <chrono>
#include <memory>
#include <string>

#include "lifecycle_msgs/msg/transition.hpp"
#include "lifecycle_msgs/srv/change_state.hpp"
#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/bool.hpp"

namespace uvms_hal_manipulator
{

class DualArmLifecycleManager : public rclcpp::Node
{
public:
    DualArmLifecycleManager()
    : Node("dual_arm_lifecycle_manager")
    {
        // 左臂 fault 订阅
        left_fault_sub_ = this->create_subscription<std_msgs::msg::Bool>(
            "/left_arm/hal/manipulator/fault",
            rclcpp::QoS(10),
            std::bind(&DualArmLifecycleManager::left_fault_callback, this, std::placeholders::_1));

        // 右臂 fault 订阅
        right_fault_sub_ = this->create_subscription<std_msgs::msg::Bool>(
            "/right_arm/hal/manipulator/fault",
            rclcpp::QoS(10),
            std::bind(&DualArmLifecycleManager::right_fault_callback, this, std::placeholders::_1));

        // 左臂 lifecycle change_state client
        left_change_state_client_ =
            this->create_client<lifecycle_msgs::srv::ChangeState>(
                "/left_arm/manipulator_driver/change_state");

        // 右臂 lifecycle change_state client
        right_change_state_client_ =
            this->create_client<lifecycle_msgs::srv::ChangeState>(
                "/right_arm/manipulator_driver/change_state");

        RCLCPP_INFO(
            this->get_logger(),
            "DualArmLifecycleManager started. Waiting for fault topics...");
    }

private:
    void left_fault_callback(const std_msgs::msg::Bool::SharedPtr msg)
    {
        if (!msg->data) {
            return;
        }

        RCLCPP_ERROR(
            this->get_logger(),
            "[FAULT] Left arm reported communication fault. Both arms will be deactivated.");
        handle_dual_arm_fault("left_arm");
    }

    void right_fault_callback(const std_msgs::msg::Bool::SharedPtr msg)
    {
        if (!msg->data) {
            return;
        }

        RCLCPP_ERROR(
            this->get_logger(),
            "[FAULT] Right arm reported communication fault. Both arms will be deactivated.");
        handle_dual_arm_fault("right_arm");
    }

    void handle_dual_arm_fault(const std::string& source_arm)
    {
        // 防止重复触发
        if (fault_latched_) {
            RCLCPP_WARN(
                this->get_logger(),
                "Fault already latched. Ignoring repeated fault from %s.",
                source_arm.c_str());
            return;
        }

        fault_latched_ = true;

        RCLCPP_ERROR(
            this->get_logger(),
            "Emergency lifecycle fallback triggered by %s. "
            "Both arms will transition from active to inactive. "
            "System will wait for manual intervention.",
            source_arm.c_str());

        deactivate_both_arms();
    }

    void deactivate_both_arms()
    {
        // 等待 service 出现
        wait_for_service_or_warn(left_change_state_client_, "/left_arm/manipulator_driver/change_state");
        wait_for_service_or_warn(right_change_state_client_, "/right_arm/manipulator_driver/change_state");

        // 左右臂统一 deactivate
        call_deactivate(left_change_state_client_, "left_arm");
        call_deactivate(right_change_state_client_, "right_arm");
    }

    void wait_for_service_or_warn(
        const rclcpp::Client<lifecycle_msgs::srv::ChangeState>::SharedPtr& client,
        const std::string& service_name)
    {
        if (!client->wait_for_service(std::chrono::seconds(2))) {
            RCLCPP_ERROR(
                this->get_logger(),
                "Lifecycle service not available: %s",
                service_name.c_str());
        }
    }

    void call_deactivate(
        const rclcpp::Client<lifecycle_msgs::srv::ChangeState>::SharedPtr& client,
        const std::string& arm_name)
    {
        if (!client->service_is_ready()) {
            RCLCPP_ERROR(
                this->get_logger(),
                "[%s] change_state service is not ready.",
                arm_name.c_str());
            return;
        }

        auto request = std::make_shared<lifecycle_msgs::srv::ChangeState::Request>();
        request->transition.id = lifecycle_msgs::msg::Transition::TRANSITION_DEACTIVATE;

        auto future = client->async_send_request(request);

        // 同步等待结果，便于日志明确
        auto ret = rclcpp::spin_until_future_complete(this->get_node_base_interface(), future);

        if (ret != rclcpp::FutureReturnCode::SUCCESS) {
            RCLCPP_ERROR(
                this->get_logger(),
                "[%s] Failed to call deactivate transition.",
                arm_name.c_str());
            return;
        }

        const auto response = future.get();
        if (response->success) {
            RCLCPP_WARN(
                this->get_logger(),
                "[%s] successfully transitioned to inactive.",
                arm_name.c_str());
        } else {
            RCLCPP_ERROR(
                this->get_logger(),
                "[%s] deactivate transition was rejected.",
                arm_name.c_str());
        }
    }

private:
    rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr left_fault_sub_;
    rclcpp::Subscription<std_msgs::msg::Bool>::SharedPtr right_fault_sub_;

    rclcpp::Client<lifecycle_msgs::srv::ChangeState>::SharedPtr left_change_state_client_;
    rclcpp::Client<lifecycle_msgs::srv::ChangeState>::SharedPtr right_change_state_client_;

    bool fault_latched_{false};
};

}  // namespace uvms_hal_manipulator

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);

    auto node = std::make_shared<uvms_hal_manipulator::DualArmLifecycleManager>();
    rclcpp::spin(node);

    rclcpp::shutdown();
    return 0;
}