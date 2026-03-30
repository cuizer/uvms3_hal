#include "manipulator_hal/manipulator_lifecycle_node.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

#include "lifecycle_msgs/msg/state.hpp"

namespace uvms_hal_manipulator
{

ManipulatorLifecycleNode::ManipulatorLifecycleNode(const rclcpp::NodeOptions& options)
: rclcpp_lifecycle::LifecycleNode("manipulator_driver", options)
{
    RCLCPP_INFO(get_logger(), "ManipulatorLifecycleNode created.");
}

auto ManipulatorLifecycleNode::on_configure(const rclcpp_lifecycle::State&) -> CallbackReturn
{
    RCLCPP_INFO(get_logger(), "[%s] on_configure()", get_name());

    declare_and_load_parameters();

    if (!init_safety_config()) {
        RCLCPP_ERROR(get_logger(), "Failed to initialize safety config.");
        return CallbackReturn::FAILURE;
    }

    if (!init_can_driver()) {
        RCLCPP_ERROR(get_logger(), "Failed to initialize CAN driver.");
        return CallbackReturn::FAILURE;
    }

    reset_runtime_state();

    // Subscriber
    joint_cmd_sub_ = create_subscription<trajectory_msgs::msg::JointTrajectoryPoint>(
        "hal/manipulator/joint_cmd",
        rclcpp::QoS(10),
        std::bind(&ManipulatorLifecycleNode::joint_cmd_callback, this, std::placeholders::_1));

    // Service
    emergency_stop_srv_ = create_service<std_srvs::srv::SetBool>(
        "hal/manipulator/emergency_stop",
        std::bind(
            &ManipulatorLifecycleNode::emergency_stop_callback,
            this,
            std::placeholders::_1,
            std::placeholders::_2));

    // Lifecycle publishers
    joint_state_pub_ =
        create_publisher<sensor_msgs::msg::JointState>("hal/manipulator/joint_states", rclcpp::QoS(10));

    ee_pose_pub_ =
        create_publisher<geometry_msgs::msg::PoseStamped>("hal/manipulator/end_effector_pose", rclcpp::QoS(10));

    status_pub_ =
        create_publisher<std_msgs::msg::String>("hal/manipulator/status", rclcpp::QoS(10));

    // Timer
    const auto period_ms =
        std::chrono::milliseconds(static_cast<int>(1000.0 / std::max(1.0, publish_rate_hz_)));

    timer_ = create_wall_timer(
        period_ms,
        std::bind(&ManipulatorLifecycleNode::timer_callback, this));

    RCLCPP_INFO(get_logger(), "[%s] configured successfully.", get_name());
    return CallbackReturn::SUCCESS;
}

auto ManipulatorLifecycleNode::on_activate(const rclcpp_lifecycle::State&) -> CallbackReturn
{
    RCLCPP_INFO(get_logger(), "[%s] on_activate()", get_name());

    joint_state_pub_->on_activate();
    ee_pose_pub_->on_activate();
    status_pub_->on_activate();

    publish_status("Manipulator HAL node activated.");
    return CallbackReturn::SUCCESS;
}

auto ManipulatorLifecycleNode::on_deactivate(const rclcpp_lifecycle::State&) -> CallbackReturn
{
    RCLCPP_INFO(get_logger(), "[%s] on_deactivate()", get_name());

    if (joint_state_pub_) {
        joint_state_pub_->on_deactivate();
    }
    if (ee_pose_pub_) {
        ee_pose_pub_->on_deactivate();
    }
    if (status_pub_) {
        status_pub_->on_deactivate();
    }

    return CallbackReturn::SUCCESS;
}

auto ManipulatorLifecycleNode::on_cleanup(const rclcpp_lifecycle::State&) -> CallbackReturn
{
    RCLCPP_INFO(get_logger(), "[%s] on_cleanup()", get_name());

    timer_.reset();
    joint_cmd_sub_.reset();
    emergency_stop_srv_.reset();

    joint_state_pub_.reset();
    ee_pose_pub_.reset();
    status_pub_.reset();

    can_driver_.close();
    reset_runtime_state();

    return CallbackReturn::SUCCESS;
}

auto ManipulatorLifecycleNode::on_shutdown(const rclcpp_lifecycle::State&) -> CallbackReturn
{
    RCLCPP_INFO(get_logger(), "[%s] on_shutdown()", get_name());
    can_driver_.close();
    return CallbackReturn::SUCCESS;
}

auto ManipulatorLifecycleNode::on_error(const rclcpp_lifecycle::State&) -> CallbackReturn
{
    RCLCPP_ERROR(get_logger(), "[%s] on_error()", get_name());
    can_driver_.close();
    return CallbackReturn::SUCCESS;
}

void ManipulatorLifecycleNode::declare_and_load_parameters()
{
    this->declare_parameter<std::string>("arm_name", "arm");
    this->declare_parameter<std::string>("can_interface", "can0");
    this->declare_parameter<std::string>("base_frame", "base_link");
    this->declare_parameter<std::string>("ee_frame", "ee_link");
    this->declare_parameter<double>("publish_rate_hz", 50.0);
    this->declare_parameter<bool>("debug_mode", false);


    this->declare_parameter<std::vector<std::string>>("joint_names", std::vector<std::string>{});
    this->declare_parameter<std::vector<double>>("joint.position_limit_min", std::vector<double>{});
    this->declare_parameter<std::vector<double>>("joint.position_limit_max", std::vector<double>{});
    this->declare_parameter<std::vector<double>>("joint.max_velocity", std::vector<double>{});

    this->declare_parameter<double>("safety.max_current", 500.0);
    this->declare_parameter<double>("safety.max_temperature", 100.0);
    this->declare_parameter<double>("safety.communication_timeout_sec", 0.2);

    arm_name_ = this->get_parameter("arm_name").as_string();
    can_interface_ = this->get_parameter("can_interface").as_string();
    base_frame_ = this->get_parameter("base_frame").as_string();
    ee_frame_ = this->get_parameter("ee_frame").as_string();
    publish_rate_hz_ = this->get_parameter("publish_rate_hz").as_double();
    debug_mode_ = this->get_parameter("debug_mode").as_bool();

    joint_names_ = this->get_parameter("joint_names").as_string_array();
    joint_pos_min_ = this->get_parameter("joint.position_limit_min").as_double_array();
    joint_pos_max_ = this->get_parameter("joint.position_limit_max").as_double_array();
    joint_vel_max_ = this->get_parameter("joint.max_velocity").as_double_array();

    max_current_ = this->get_parameter("safety.max_current").as_double();
    max_temperature_ = this->get_parameter("safety.max_temperature").as_double();
    comm_timeout_sec_ = this->get_parameter("safety.communication_timeout_sec").as_double();

    RCLCPP_INFO(
        get_logger(),
        "Loaded parameters: arm_name=%s, can_interface=%s, publish_rate=%.2f",
        arm_name_.c_str(), can_interface_.c_str(), publish_rate_hz_);
}

bool ManipulatorLifecycleNode::init_safety_config()
{
    JointLimitConfig joint_cfg;
    joint_cfg.position_min = joint_pos_min_;
    joint_cfg.position_max = joint_pos_max_;
    joint_cfg.max_velocity = joint_vel_max_;

    MotorSafetyConfig motor_cfg;
    motor_cfg.max_current = max_current_;
    motor_cfg.max_temperature = max_temperature_;
    motor_cfg.communication_timeout_sec = comm_timeout_sec_;

    safety_manager_.set_joint_limit_config(joint_cfg);
    safety_manager_.set_motor_safety_config(motor_cfg);

    return true;
}

bool ManipulatorLifecycleNode::init_can_driver()
{
    if (!can_driver_.open(can_interface_)) {
        return false;
    }
    can_driver_.flush();
    return true;
}

void ManipulatorLifecycleNode::reset_runtime_state()
{
    communication_ok_ = false;
    last_rx_time_ = this->now();

    latest_joint_position_.assign(joint_names_.size(), 0.0);
    latest_joint_velocity_.assign(joint_names_.size(), 0.0);
    latest_joint_effort_.assign(joint_names_.size(), 0.0);
}

void ManipulatorLifecycleNode::joint_cmd_callback(
    const trajectory_msgs::msg::JointTrajectoryPoint::SharedPtr msg)
{
    if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) {
        return;
    }

    std::vector<double> positions(msg->positions.begin(), msg->positions.end());
    std::vector<double> velocities(msg->velocities.begin(), msg->velocities.end());

    const auto check_result = safety_manager_.validate_joint_command(positions, velocities);
    if (!check_result.ok) {
        RCLCPP_WARN(get_logger(), "Joint command rejected: %s", check_result.reason.c_str());
        publish_status("Joint command rejected: " + check_result.reason);
        return;
    }

    // 当前通用版协议只定义了 #16 控制指令，不含完整关节控制帧
    // 因此这里先保留为占位结构，后续待真实 CAN 关节协议明确后扩展。
    //
    // 目前做法：收到合法命令时仅更新本地缓存，并发布状态提示。
    latest_joint_position_ = positions;
    latest_joint_velocity_ = safety_manager_.clamp_velocity(velocities);

    // publish_status("Joint command accepted (placeholder, waiting for detailed CAN motion protocol).");
    // =========================
// DEBUG BLOCK BEGIN
// 调试模式下收到合法 joint_cmd 后直接回显，便于验证 ROS 接口链路
// 后期不调试时可直接删除本块，或保留普通状态提示
// =========================
   if (debug_mode_) {
       std::ostringstream oss;
       oss << "DEBUG: joint_cmd accepted, positions=[";
       for (size_t i = 0; i < positions.size(); ++i) {
           oss << positions[i];
           if (i + 1 < positions.size()) {
               oss << ", ";
           }
       }
       oss << "], velocities=[";
       for (size_t i = 0; i < velocities.size(); ++i) {
           oss << velocities[i];
           if (i + 1 < velocities.size()) {
               oss << ", ";
           }
       }
       oss << "]";
       publish_status(oss.str());
   } else {
       publish_status("Joint command accepted (placeholder, waiting for detailed CAN motion protocol).");
   }
// =========================
// DEBUG BLOCK END
// =========================
}

void ManipulatorLifecycleNode::emergency_stop_callback(
    const std::shared_ptr<std_srvs::srv::SetBool::Request> request,
    std::shared_ptr<std_srvs::srv::SetBool::Response> response)
{
    safety_manager_.set_estop(request->data);

    if (request->data) {
        response->success = true;
        response->message = "Emergency stop activated.";
        publish_status("Emergency stop activated.");
    } else {
        response->success = true;
        response->message = "Emergency stop released.";
        publish_status("Emergency stop released.");
    }
}

void ManipulatorLifecycleNode::timer_callback()
{
    if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) {
        return;
    }

    // 1. 读取 CAN 状态帧
    CanFrame frame;
    bool received = false;

    // 一次 timer 中尽量多处理几帧，避免缓存积压
    for (int i = 0; i < 20; ++i) {
        if (!can_driver_.read_frame(frame)) {
            break;
        }
        received = true;
        process_rx_frame(frame);
    }

    if (received) {
        last_rx_time_ = this->now();
        communication_ok_ = true;
    }

    // 2. 通信超时检查
    // const double elapsed =
    //     (this->now() - last_rx_time_).seconds();
    // const auto comm_result = safety_manager_.check_communication_timeout(elapsed);
    // if (!comm_result.ok) {
    //     communication_ok_ = false;
    //     publish_status(comm_result.reason);
    // }
    // =========================
// DEBUG BLOCK BEGIN
// 调试模式下忽略通信超时刷屏，避免无真实硬件时持续报 timeout
// 后期不调试时可直接删除本块，或将 debug_mode 置为 false
// =========================
   if (!debug_mode_) {
       const double elapsed =
           (this->now() - last_rx_time_).seconds();
       const auto comm_result = safety_manager_.check_communication_timeout(elapsed);
       if (!comm_result.ok) {
           communication_ok_ = false;
           publish_status(comm_result.reason);
       }
   } else {
       communication_ok_ = true;
   }
// =========================
// DEBUG BLOCK END
// =========================

    // 3. 电机过载检查（通用版先使用 arm_motor_state）
    auto currents = int16_array_to_double_vector_10(latest_arm_motor_state_.current);
    auto temperatures = uint16_array_to_double_vector_10(latest_arm_motor_state_.temperature);

    const auto overload_result = safety_manager_.check_motor_overload(currents, temperatures);
    if (!overload_result.ok) {
        publish_status(overload_result.reason);
    }

    // 4. 发布状态
    publish_joint_states();
    publish_end_effector_pose();
}

bool ManipulatorLifecycleNode::process_rx_frame(const CanFrame& frame)
{
    const auto complete_msg = protocol_parser_.process_can_frame(frame);
    if (!complete_msg.has_value()) {
        return false;
    }

    if (complete_msg->type == CompleteMessageType::ARMCABIN_MOTOR) {
        protocol_parser_.get_armcabin_motor_state(latest_armcabin_motor_state_);
    } else if (complete_msg->type == CompleteMessageType::ARM_MOTOR) {
        protocol_parser_.get_arm_motor_state(latest_arm_motor_state_);

        // 通用映射：前 joint_names_.size() 个电机位置/速度映射到 joint_states
        const size_t n = std::min(joint_names_.size(), latest_joint_position_.size());
        for (size_t i = 0; i < n && i < latest_arm_motor_state_.position.size(); ++i) {
            latest_joint_position_[i] = static_cast<double>(latest_arm_motor_state_.position[i]);
            latest_joint_velocity_[i] = static_cast<double>(latest_arm_motor_state_.speed[i]);
            latest_joint_effort_[i] = static_cast<double>(latest_arm_motor_state_.current[i]);
        }
    } else if (complete_msg->type == CompleteMessageType::ARM_CONTROLLER) {
        protocol_parser_.get_arm_controller_state(latest_arm_controller_state_);
    }

    return true;
}

void ManipulatorLifecycleNode::publish_joint_states()
{
    if (!joint_state_pub_ || !joint_state_pub_->is_activated()) {
        return;
    }

    sensor_msgs::msg::JointState msg;
    msg.header.stamp = this->now();
    msg.name = joint_names_;
    msg.position = latest_joint_position_;
    msg.velocity = latest_joint_velocity_;
    msg.effort = latest_joint_effort_;

    joint_state_pub_->publish(msg);
}

void ManipulatorLifecycleNode::publish_end_effector_pose()
{
    if (!ee_pose_pub_ || !ee_pose_pub_->is_activated()) {
        return;
    }

    // 当前版本先给出占位式末端位姿：
    // 真实系统中，这里应根据 latest_joint_position_ 做正运动学求解。
    geometry_msgs::msg::PoseStamped msg;
    msg.header.stamp = this->now();
    msg.header.frame_id = base_frame_;

    msg.pose.position.x = 0.0;
    msg.pose.position.y = 0.0;
    msg.pose.position.z = 0.0;
    msg.pose.orientation.x = 0.0;
    msg.pose.orientation.y = 0.0;
    msg.pose.orientation.z = 0.0;
    msg.pose.orientation.w = 1.0;

    ee_pose_pub_->publish(msg);
}

void ManipulatorLifecycleNode::publish_status(const std::string& text)
{
    if (!status_pub_ || !status_pub_->is_activated()) {
        return;
    }

    std_msgs::msg::String msg;
    msg.data = "[" + arm_name_ + "] " + text;
    status_pub_->publish(msg);
}

std::vector<double> ManipulatorLifecycleNode::int16_array_to_double_vector_2(
    const std::array<int16_t, 2>& arr) const
{
    std::vector<double> v(arr.size(), 0.0);
    for (size_t i = 0; i < arr.size(); ++i) {
        v[i] = static_cast<double>(arr[i]);
    }
    return v;
}

std::vector<double> ManipulatorLifecycleNode::int16_array_to_double_vector_10(
    const std::array<int16_t, 10>& arr) const
{
    std::vector<double> v(arr.size(), 0.0);
    for (size_t i = 0; i < arr.size(); ++i) {
        v[i] = static_cast<double>(arr[i]);
    }
    return v;
}

std::vector<double> ManipulatorLifecycleNode::uint16_array_to_double_vector_10(
    const std::array<uint16_t, 10>& arr) const
{
    std::vector<double> v(arr.size(), 0.0);
    for (size_t i = 0; i < arr.size(); ++i) {
        v[i] = static_cast<double>(arr[i]);
    }
    return v;
}

}  // namespace uvms_hal_manipulator