#include "uvms_hal_manipulator/dual_arm_manager.hpp"
#include <chrono>

namespace uvms_hal_manipulator
{

using namespace std::chrono_literals;

DualArmLifecycleManager::DualArmLifecycleManager(const rclcpp::NodeOptions &options)
    : Node("dual_arm_lifecycle_manager", options)
{
    // 左臂故障订阅
    left_fault_sub_ = this->create_subscription<std_msgs::msg::Bool>(
        "/left_arm/hal/manipulator/fault",
        rclcpp::QoS(10),
        std::bind(&DualArmLifecycleManager::left_fault_callback, this, std::placeholders::_1));

    // 右臂故障订阅
    right_fault_sub_ = this->create_subscription<std_msgs::msg::Bool>(
        "/right_arm/hal/manipulator/fault",
        rclcpp::QoS(10),
        std::bind(&DualArmLifecycleManager::right_fault_callback, this, std::placeholders::_1));

    // 生命周期客户端
    left_change_state_client_ = this->create_client<lifecycle_msgs::srv::ChangeState>(
        "/left_arm/manipulator_driver/change_state");

    right_change_state_client_ = this->create_client<lifecycle_msgs::srv::ChangeState>(
        "/right_arm/manipulator_driver/change_state");

    RCLCPP_INFO(this->get_logger(), "DualArmLifecycleManager 启动完成，等待双臂信号...");
}

void DualArmLifecycleManager::left_fault_callback(const std_msgs::msg::Bool::SharedPtr msg)
{
    if (!msg->data) return;

    RCLCPP_ERROR(this->get_logger(), "[故障] 左臂发生通信故障，系统将停机双臂！");
    handle_dual_arm_fault("left_arm");
}

void DualArmLifecycleManager::right_fault_callback(const std_msgs::msg::Bool::SharedPtr msg)
{
    if (!msg->data) return;

    RCLCPP_ERROR(this->get_logger(), "[故障] 右臂发生通信故障，系统将停机双臂！");
    handle_dual_arm_fault("right_arm");
}

void DualArmLifecycleManager::handle_dual_arm_fault(const std::string &source_arm)
{
    if (fault_latched_) {
        RCLCPP_WARN(this->get_logger(), "故障已锁存，忽略重复触发");
        return;
    }

    fault_latched_ = true;
    deactivate_both_arms();
}

void DualArmLifecycleManager::deactivate_both_arms()
{
    wait_for_service_or_warn(left_change_state_client_, "/left_arm/manipulator_driver/change_state");
    wait_for_service_or_warn(right_change_state_client_, "/right_arm/manipulator_driver/change_state");

    call_deactivate(left_change_state_client_, "left_arm");
    call_deactivate(right_change_state_client_, "right_arm");

    RCLCPP_ERROR(this->get_logger(), "双臂已全部停机，需人工复位才能重新激活！");
}

void DualArmLifecycleManager::wait_for_service_or_warn(
    const rclcpp::Client<lifecycle_msgs::srv::ChangeState>::SharedPtr &client,
    const std::string &service_name)
{
    if (!client->wait_for_service(2s)) {
        RCLCPP_ERROR(this->get_logger(), "服务不存在: %s", service_name.c_str());
    }
}

void DualArmLifecycleManager::call_deactivate(
    const rclcpp::Client<lifecycle_msgs::srv::ChangeState>::SharedPtr &client,
    const std::string &arm_name)
{
    if (!client->service_is_ready()) {
        RCLCPP_ERROR(this->get_logger(), "[%s] 服务未就绪", arm_name.c_str());
        return;
    }

    auto req = std::make_shared<lifecycle_msgs::srv::ChangeState::Request>();
    req->transition.id = lifecycle_msgs::msg::Transition::TRANSITION_DEACTIVATE;

    auto future = client->async_send_request(req);
    auto ret = rclcpp::spin_until_future_complete(this->get_node_base_interface(), future);

    if (ret != rclcpp::FutureReturnCode::SUCCESS) {
        RCLCPP_ERROR(this->get_logger(), "[%s] 停机调用失败", arm_name.c_str());
        return;
    }

    auto resp = future.get();
    if (resp->success) {
        RCLCPP_WARN(this->get_logger(), "[%s] 已成功停机(inactive)", arm_name.c_str());
    } else {
        RCLCPP_ERROR(this->get_logger(), "[%s] 停机失败", arm_name.c_str());
    }
}

}  // namespace uvms_hal_manipulator

// 主函数
int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<uvms_hal_manipulator::DualArmLifecycleManager>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
