#include "uvms_hal_manipulator/manipulator_node.hpp"
#include "lifecycle_msgs/msg/state.hpp"

#include <rclcpp/executors/single_threaded_executor.hpp>
#include <algorithm>
#include <chrono>
#include <cmath>
#include <vector>
#include <thread>
#include <map>
#include <iostream>
#include <cstdint>
#include <array>
#include "hal/msg/can_frame_manipulator.hpp"

namespace uvms_hal_manipulator
{

ManipulatorLifecycleNode::ManipulatorLifecycleNode(const rclcpp::NodeOptions& options)
: rclcpp_lifecycle::LifecycleNode("manipulator_driver", options),
  arm_name_("arm"),
  can_interface_("can4"),
  base_frame_("base_link"),
  ee_frame_("ee_link"),
  arm_side_("left"),
  last_rx_time_(this->now()),
  communication_ok_(false),
  initial_pose_complete_(false),
  control_enabled_(false),
  communication_lost_latched_(false),
  fault_stop_requested_(false),
  data_upload_enabled_(true)
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

    reset_runtime_state();

    // ============================================================
    // CAN manager 架构：
    // manipulator_driver 不再直接 open/read/write can0。
    // CAN 请求统一发布到 /hal/can_tx。
    // CAN 反馈统一订阅 /hal/can_rx。
    // ============================================================
    can_tx_pub_ = create_publisher<hal::msg::CanFrameManipulator>(
        "/hal/can_tx",
        rclcpp::QoS(200));
    can_tx_pub_->on_activate();

    can_rx_sub_ = create_subscription<hal::msg::CanFrameManipulator>(
        "/hal/can_rx",
        rclcpp::QoS(200),
        [this](hal::msg::CanFrameManipulator::SharedPtr msg)
        {
            this->can_rx_callback(msg);
        });

    using std::placeholders::_1;

    can_rx_sub_ = this->create_subscription<hal::msg::CanFrameManipulator>(
        "/hal/can_rx",
        rclcpp::QoS(10),
        std::bind(&ManipulatorLifecycleNode::can_rx_callback, this, _1)
    );

    joint_cmd_sub_ = create_subscription<trajectory_msgs::msg::JointTrajectoryPoint>(
        "hal/manipulator/joint_cmd",
        rclcpp::QoS(10),
        std::bind(&ManipulatorLifecycleNode::joint_cmd_callback, this, std::placeholders::_1));

    emergency_stop_srv_ = create_service<std_srvs::srv::SetBool>(
        "hal/manipulator/emergency_stop",
        std::bind(&ManipulatorLifecycleNode::emergency_stop_callback, this,
                  std::placeholders::_1, std::placeholders::_2));

    joint_state_pub_ = create_publisher<sensor_msgs::msg::JointState>(
        "hal/manipulator/joint_states", rclcpp::QoS(10));

    ee_pose_pub_ = create_publisher<geometry_msgs::msg::PoseStamped>(
        "hal/manipulator/end_effector_pose", rclcpp::QoS(10));

    status_pub_ = create_publisher<std_msgs::msg::String>(
        "hal/manipulator/status", rclcpp::QoS(10));

    fault_pub_ = create_publisher<std_msgs::msg::Bool>(
        "hal/manipulator/fault", rclcpp::QoS(10));

    armmotor_state_pub_ = create_publisher<hal::msg::HalArmmotor>(
        "hal/armmotor", rclcpp::QoS(10));

    armmotor_cmd_srv_ = create_service<hal::srv::HalArmmotorSrv>(
        "hal/armmotor_cmd",
        std::bind(&ManipulatorLifecycleNode::armmotor_cmd_callback, this,
                  std::placeholders::_1, std::placeholders::_2));

    auto period_ms = std::chrono::milliseconds(
        static_cast<int>(1000.0 / std::max(1.0, publish_rate_hz_)));

    timer_ = create_wall_timer(
        period_ms,
        std::bind(&ManipulatorLifecycleNode::timer_callback, this));

    RCLCPP_INFO(get_logger(), "[%s] configured successfully.", get_name());
    return CallbackReturn::SUCCESS;
}

auto ManipulatorLifecycleNode::on_activate(const rclcpp_lifecycle::State&) -> CallbackReturn
{
    if (!initial_pose_complete_) {
        RCLCPP_ERROR(get_logger(), "Cannot activate: initial pose not ready!");
        return CallbackReturn::FAILURE;
    }

    RCLCPP_INFO(get_logger(), "[%s] on_activate()", get_name());

    joint_state_pub_->on_activate();
    ee_pose_pub_->on_activate();
    status_pub_->on_activate();
    fault_pub_->on_activate();

    if (armmotor_state_pub_) {
        armmotor_state_pub_->on_activate();
    }

    if (latest_joint_position_.size() == joint_names_.size()) {
        target_joint_position_ = latest_joint_position_;

        RCLCPP_INFO(get_logger(), "Initialize target position with current joint position.");

        for (size_t i = 0; i < joint_names_.size(); ++i) {
            RCLCPP_INFO(
                get_logger(),
                "  hold %s: %.3f rad, %.2f deg",
                joint_names_[i].c_str(),
                target_joint_position_[i],
                target_joint_position_[i] * 180.0 / M_PI);
        }
    } else {
        RCLCPP_ERROR(
            get_logger(),
            "Cannot activate: latest_joint_position_ size (%zu) != joint_names size (%zu).",
            latest_joint_position_.size(),
            joint_names_.size());
        return CallbackReturn::FAILURE;
    }

    control_enabled_ = true;
    fault_stop_requested_ = false;
    send_position_log_printed_ = false;

    publish_status("Manipulator activated.");

    return CallbackReturn::SUCCESS;
}

auto ManipulatorLifecycleNode::on_deactivate(const rclcpp_lifecycle::State&) -> CallbackReturn
{
    RCLCPP_INFO(get_logger(), "[%s] on_deactivate()", get_name());

    if (joint_state_pub_) joint_state_pub_->on_deactivate();
    if (ee_pose_pub_) ee_pose_pub_->on_deactivate();
    if (status_pub_) status_pub_->on_deactivate();
    if (fault_pub_) fault_pub_->on_deactivate();

    if (armmotor_state_pub_) {
        armmotor_state_pub_->on_deactivate();
    }

    control_enabled_ = false;
    return CallbackReturn::SUCCESS;
}

auto ManipulatorLifecycleNode::on_cleanup(const rclcpp_lifecycle::State&) -> CallbackReturn
{
    RCLCPP_INFO(get_logger(), "[%s] on_cleanup()", get_name());

    timer_.reset();
    joint_cmd_sub_.reset();
    emergency_stop_srv_.reset();
    armmotor_cmd_srv_.reset();

    joint_state_pub_.reset();
    ee_pose_pub_.reset();
    status_pub_.reset();
    fault_pub_.reset();
    armmotor_state_pub_.reset();

    can_tx_pub_.reset();
    can_rx_sub_.reset();

    reset_runtime_state();

    return CallbackReturn::SUCCESS;
}

auto ManipulatorLifecycleNode::on_shutdown(const rclcpp_lifecycle::State&) -> CallbackReturn
{
    RCLCPP_INFO(get_logger(), "[%s] on_shutdown()", get_name());
    // can_driver_.close();
    return CallbackReturn::SUCCESS;
}

auto ManipulatorLifecycleNode::on_error(const rclcpp_lifecycle::State&) -> CallbackReturn
{
    RCLCPP_ERROR(get_logger(), "[%s] on_error()", get_name());
    return CallbackReturn::SUCCESS;
}

void ManipulatorLifecycleNode::publish_can_frame(
    uint32_t can_id,
    uint8_t dlc,
    const std::array<uint8_t, 8>& data,
    uint8_t priority,
    const std::string& frame_type)
{
    if (!can_tx_pub_) {
        RCLCPP_WARN_THROTTLE(
            get_logger(),
            *get_clock(),
            2000,
            "CAN tx publisher is not initialized.");
        return;
    }

    hal::msg::CanFrameManipulator msg;
    msg.stamp = this->now();
    msg.can_id = can_id;
    msg.dlc = dlc;
    msg.priority = priority;
    msg.source = arm_side_;
    msg.frame_type = frame_type;

    for (size_t i = 0; i < 8; ++i) {
        msg.data[i] = data[i];
    }

    can_tx_pub_->publish(msg);
}

void ManipulatorLifecycleNode::declare_and_load_parameters()
{
    this->declare_parameter<std::string>("arm_name", "arm");
    this->declare_parameter<std::string>("can_interface", "can4");
    this->declare_parameter<std::string>("base_frame", "base_link");
    this->declare_parameter<std::string>("ee_frame", "ee_link");
    this->declare_parameter<double>("publish_rate_hz", 50.0);
    this->declare_parameter<bool>("debug_mode", true);

    this->declare_parameter<std::string>("arm_side", "left");
    this->declare_parameter<bool>("require_initial_pose_before_activate", true);
    this->declare_parameter<bool>("fault_stop_on_comm_loss", true);

    // ============================================================
    // 单臂默认 5 个电机
    //
    // 左臂 YAML 建议：
    // motor_can_ids: [1, 3, 5, 7, 9]
    //
    // 右臂 YAML 建议：
    // motor_can_ids: [2, 4, 6, 8, 10]
    //
    // 注意：
    // 每一个 manipulator_driver 节点只管理一只机械臂的 5 个电机，
    // 不是一个节点直接管理双臂 10 个电机。
    // ============================================================
    this->declare_parameter<std::vector<int64_t>>(
        "motor_can_ids",
        {1, 3, 5, 7, 9});

    this->declare_parameter<std::vector<std::string>>(
        "joint_names",
        {"joint1", "joint2", "joint3", "joint4", "joint5"});

    this->declare_parameter<std::vector<double>>(
        "joint.position_limit_min",
        {-3.14159, -3.14159, -3.14159, -3.14159, -3.14159});

    this->declare_parameter<std::vector<double>>(
        "joint.position_limit_max",
        {3.14159, 3.14159, 3.14159, 3.14159, 3.14159});

    // 单位通常为 rad/s。
    // 1.0 rad/s 约等于 57.3 deg/s。
    // 当前 HAL 主要使用位置控制，此参数更多用于后续 BSP 轨迹规划或速度限幅。
    this->declare_parameter<std::vector<double>>(
        "joint.max_velocity",
        {1.0, 1.0, 1.0, 1.0, 1.0});

    this->declare_parameter<double>("safety.max_current", 500.0);
    this->declare_parameter<double>("safety.max_temperature", 80.0);
    this->declare_parameter<double>("safety.communication_timeout_sec", 0.5);

    arm_name_ = this->get_parameter("arm_name").as_string();
    can_interface_ = this->get_parameter("can_interface").as_string();
    base_frame_ = this->get_parameter("base_frame").as_string();
    ee_frame_ = this->get_parameter("ee_frame").as_string();
    publish_rate_hz_ = this->get_parameter("publish_rate_hz").as_double();
    debug_mode_ = this->get_parameter("debug_mode").as_bool();

    arm_side_ = this->get_parameter("arm_side").as_string();

    require_initial_pose_before_activate_ =
        this->get_parameter("require_initial_pose_before_activate").as_bool();

    fault_stop_on_comm_loss_ =
        this->get_parameter("fault_stop_on_comm_loss").as_bool();

    motor_can_ids_param_ =
        this->get_parameter("motor_can_ids").as_integer_array();

    joint_names_ =
        this->get_parameter("joint_names").as_string_array();

    joint_pos_min_ =
        this->get_parameter("joint.position_limit_min").as_double_array();

    joint_pos_max_ =
        this->get_parameter("joint.position_limit_max").as_double_array();

    joint_vel_max_ =
        this->get_parameter("joint.max_velocity").as_double_array();

    max_current_ =
        this->get_parameter("safety.max_current").as_double();

    max_temperature_ =
        this->get_parameter("safety.max_temperature").as_double();

    comm_timeout_sec_ =
        this->get_parameter("safety.communication_timeout_sec").as_double();

    RCLCPP_INFO(
        get_logger(),
        "Parameters loaded: arm_name=%s, arm_side=%s, can_interface=%s, motor_count=%zu, joint_count=%zu",
        arm_name_.c_str(),
        arm_side_.c_str(),
        can_interface_.c_str(),
        motor_can_ids_param_.size(),
        joint_names_.size());
}

bool ManipulatorLifecycleNode::init_safety_config()
{
    // ============================================================
    // 注意：
    // 参数已经在 declare_and_load_parameters() 中读取完成。
    // 这里不再重复 get_parameter，只负责检查参数是否合法。
    // ============================================================

    if (max_current_ <= 0.0) {
        RCLCPP_ERROR(
            get_logger(),
            "Safety max_current must be > 0, current value: %.2f",
            max_current_);
        return false;
    }

    if (max_temperature_ <= 0.0) {
        RCLCPP_ERROR(
            get_logger(),
            "Safety max_temperature must be > 0, current value: %.2f",
            max_temperature_);
        return false;
    }

    if (comm_timeout_sec_ <= 0.0) {
        RCLCPP_ERROR(
            get_logger(),
            "Communication timeout must be > 0, current value: %.3f",
            comm_timeout_sec_);
        return false;
    }

    if (motor_can_ids_param_.empty()) {
        RCLCPP_ERROR(
            get_logger(),
            "motor_can_ids is empty. At least one motor id is required.");
        return false;
    }

    if (joint_names_.empty()) {
        RCLCPP_ERROR(
            get_logger(),
            "joint_names is empty. At least one joint name is required.");
        return false;
    }

    if (motor_can_ids_param_.size() != joint_names_.size()) {
        RCLCPP_ERROR(
            get_logger(),
            "motor_can_ids size (%zu) must match joint_names size (%zu).",
            motor_can_ids_param_.size(),
            joint_names_.size());
        return false;
    }

    if (joint_pos_min_.size() != joint_names_.size()) {
        RCLCPP_ERROR(
            get_logger(),
            "joint.position_limit_min size (%zu) must match joint_names size (%zu).",
            joint_pos_min_.size(),
            joint_names_.size());
        return false;
    }

    if (joint_pos_max_.size() != joint_names_.size()) {
        RCLCPP_ERROR(
            get_logger(),
            "joint.position_limit_max size (%zu) must match joint_names size (%zu).",
            joint_pos_max_.size(),
            joint_names_.size());
        return false;
    }

    if (joint_vel_max_.size() != joint_names_.size()) {
        RCLCPP_ERROR(
            get_logger(),
            "joint.max_velocity size (%zu) must match joint_names size (%zu).",
            joint_vel_max_.size(),
            joint_names_.size());
        return false;
    }

    for (size_t i = 0; i < joint_names_.size(); ++i) {
        if (joint_pos_min_[i] >= joint_pos_max_[i]) {
            RCLCPP_ERROR(
                get_logger(),
                "Invalid joint limit for %s: min %.3f >= max %.3f",
                joint_names_[i].c_str(),
                joint_pos_min_[i],
                joint_pos_max_[i]);
            return false;
        }

        if (joint_vel_max_[i] <= 0.0) {
            RCLCPP_ERROR(
                get_logger(),
                "Invalid max velocity for %s: %.3f",
                joint_names_[i].c_str(),
                joint_vel_max_[i]);
            return false;
        }
    }

    RCLCPP_INFO(
        get_logger(),
        "Safety configuration initialized: max_current=%.2f, max_temperature=%.2f, comm_timeout=%.3f, motors=%zu",
        max_current_,
        max_temperature_,
        comm_timeout_sec_,
        motor_can_ids_param_.size());

    return true;
}


void ManipulatorLifecycleNode::reset_runtime_state()
{
    communication_ok_ = false;
    last_rx_time_ = this->now();

    last_position_query_time_ = this->now();
    last_error_query_time_ = this->now();
    last_temp_query_time_ = this->now();

    // 根据 joint_names_ 的数量初始化关节状态数组。
    // 单臂 5 电机时，这些数组长度应该都是 5。
    latest_joint_position_.assign(joint_names_.size(), 0.0);
    latest_joint_velocity_.assign(joint_names_.size(), 0.0);
    latest_joint_effort_.assign(joint_names_.size(), 0.0);
    target_joint_position_.assign(joint_names_.size(), 0.0);

    initial_pose_complete_ = false;
    control_enabled_ = false;
    communication_lost_latched_ = false;
    fault_stop_requested_ = false;

    initial_pose_printed_map_.clear();

    latest_motor_current_.clear();
    latest_motor_speed_.clear();
    latest_motor_position_.clear();
    latest_motor_temp_.clear();
    latest_motor_error_.clear();
    latest_motor_rx_time_.clear();
    latest_motor_comm_error_.clear();

    // 多电机映射表清空。
    // build_expected_motor_id_list() 会根据 motor_can_ids_param_ 重新建立。
    motor_id_to_joint_index_.clear();

    send_position_log_printed_ = false;
    initial_pose_complete_log_printed_ = false;

    build_expected_motor_id_list();
}

void ManipulatorLifecycleNode::build_expected_motor_id_list()
{
    expected_motor_ids_.clear();
    motor_ready_map_.clear();
    initial_pose_printed_map_.clear();
    motor_id_to_joint_index_.clear();

    latest_motor_current_.clear();
    latest_motor_speed_.clear();
    latest_motor_position_.clear();
    latest_motor_temp_.clear();
    latest_motor_error_.clear();

    if (motor_can_ids_param_.empty()) {
        RCLCPP_ERROR(get_logger(), "motor_can_ids is empty, no motor will be managed.");
        return;
    }

    if (motor_can_ids_param_.size() != joint_names_.size()) {
        RCLCPP_ERROR(
            get_logger(),
            "motor_can_ids size (%zu) does not match joint_names size (%zu).",
            motor_can_ids_param_.size(),
            joint_names_.size());
        return;
    }

    // ============================================================
    // 根据 YAML 中的 motor_can_ids 建立当前节点管理的电机表。
    //
    // 左臂示例：
    // motor_can_ids: [1, 3, 5, 7, 9]
    //
    // 右臂示例：
    // motor_can_ids: [2, 4, 6, 8, 10]
    //
    // 映射关系：
    // motor_id_to_joint_index_[CAN_ID] = 关节数组下标
    // ============================================================
    for (size_t i = 0; i < motor_can_ids_param_.size(); ++i) {
        int64_t raw_id = motor_can_ids_param_[i];

        if (raw_id <= 0 || raw_id > 2047) {
            RCLCPP_ERROR(
                get_logger(),
                "Invalid CAN motor id: %ld. CAN id should be in range [1, 2047].",
                static_cast<long>(raw_id));
            continue;
        }

        uint32_t id = static_cast<uint32_t>(raw_id);

        if (motor_id_to_joint_index_.find(id) != motor_id_to_joint_index_.end()) {
            RCLCPP_ERROR(
                get_logger(),
                "Duplicate CAN motor id detected: %u. Please check motor_can_ids in YAML.",
                id);
            continue;
        }

        expected_motor_ids_.push_back(id);

        // 例如左臂：
        // id=1 -> i=0
        // id=3 -> i=1
        // id=5 -> i=2
        // id=7 -> i=3
        // id=9 -> i=4
        motor_id_to_joint_index_[id] = i;

        motor_ready_map_[id] = false;
        initial_pose_printed_map_[id] = false;

        latest_motor_current_[id] = 0;
        latest_motor_speed_[id] = 0;
        latest_motor_position_[id] = 0;
        latest_motor_temp_[id] = 0;
        latest_motor_error_[id] = 0;
        latest_motor_rx_time_[id] = this->now();
        latest_motor_comm_error_[id] = 1;
    }

    RCLCPP_INFO(
        get_logger(),
        "Motor list initialized for %s arm. motor_count=%zu, joint_count=%zu",
        arm_side_.c_str(),
        expected_motor_ids_.size(),
        joint_names_.size());

    for (size_t i = 0; i < expected_motor_ids_.size(); ++i) {
        RCLCPP_INFO(
            get_logger(),
            "  motor_id=%u -> joint_index=%zu, joint_name=%s",
            expected_motor_ids_[i],
            i,
            joint_names_[i].c_str());
    }
}

bool ManipulatorLifecycleNode::is_my_motor_id(uint32_t can_id) const
{
    for (uint32_t id : expected_motor_ids_) {
        if (id == can_id) {
            return true;
        }
    }

    return false;
}

void ManipulatorLifecycleNode::update_initial_pose_completion()
{
    if (motor_ready_map_.empty()) {
        initial_pose_complete_ = false;
        return;
    }

    bool all_ready = true;

    for (const auto& p : motor_ready_map_) {
        if (!p.second) {
            all_ready = false;
            break;
        }
    }

    initial_pose_complete_ = all_ready;

    if (initial_pose_complete_ && !initial_pose_complete_log_printed_) {
        RCLCPP_INFO(
            get_logger(),
            "✅ All motor initial positions received. Ready to activate.");
        initial_pose_complete_log_printed_ = true;
    }
}

void ManipulatorLifecycleNode::joint_cmd_callback(
    const trajectory_msgs::msg::JointTrajectoryPoint::SharedPtr msg)
{
    if (!control_enabled_ || fault_stop_requested_) {
        return;
    }

    if (msg->positions.empty()) {
        RCLCPP_WARN(get_logger(), "❌ Joint command rejected: positions is empty.");
        return;
    }

    if (msg->positions.size() != joint_names_.size()) {
        RCLCPP_WARN(
            get_logger(),
            "❌ Joint command rejected: expected %zu positions, got %zu.",
            joint_names_.size(),
            msg->positions.size());
        return;
    }

    if (target_joint_position_.size() != joint_names_.size()) {
        RCLCPP_ERROR(
            get_logger(),
            "target_joint_position_ size (%zu) does not match joint_names size (%zu).",
            target_joint_position_.size(),
            joint_names_.size());
        return;
    }

    if (joint_pos_min_.size() != joint_names_.size() ||
        joint_pos_max_.size() != joint_names_.size()) {
        RCLCPP_ERROR(
            get_logger(),
            "Joint limit size mismatch. min=%zu, max=%zu, joints=%zu.",
            joint_pos_min_.size(),
            joint_pos_max_.size(),
            joint_names_.size());
        return;
    }

    // 先逐个检查限位，全部合法后再统一写入目标。
    // 这样可以避免前几个关节已更新、后一个关节越界导致目标部分更新。
    for (size_t i = 0; i < joint_names_.size(); ++i) {
        double target = msg->positions[i];

        if (target < joint_pos_min_[i] || target > joint_pos_max_[i]) {
            RCLCPP_WARN(
                get_logger(),
                "❌ Command rejected: joint %s target %.3f rad out of limit [%.3f, %.3f].",
                joint_names_[i].c_str(),
                target,
                joint_pos_min_[i],
                joint_pos_max_[i]);
            return;
        }
    }

    for (size_t i = 0; i < joint_names_.size(); ++i) {
        target_joint_position_[i] = msg->positions[i];
    }

    RCLCPP_INFO(
        get_logger(),
        "✅ Joint command received for %zu joints.",
        joint_names_.size());

    if (debug_mode_) {
        for (size_t i = 0; i < joint_names_.size(); ++i) {
            RCLCPP_INFO(
                get_logger(),
                "  %s: %.3f rad, %.2f deg",
                joint_names_[i].c_str(),
                target_joint_position_[i],
                target_joint_position_[i] * 180.0 / M_PI);
        }
    }

    // 收到新的目标后，允许下一次 timer_callback 打印一次发送日志。
    send_position_log_printed_ = false;
}

void ManipulatorLifecycleNode::emergency_stop_callback(
    const std::shared_ptr<std_srvs::srv::SetBool::Request> req,
    std::shared_ptr<std_srvs::srv::SetBool::Response> res)
{
    if (req->data) {
        RCLCPP_ERROR(get_logger(), "EMERGENCY STOP TRIGGERED");
        fault_stop_requested_ = true;
        control_enabled_ = false;

        res->success = true;
        res->message = "Emergency stop triggered.";
        return;
    }

    res->success = true;
    res->message = "Emergency stop request ignored because data=false.";
}

void ManipulatorLifecycleNode::armmotor_cmd_callback(
    const std::shared_ptr<hal::srv::HalArmmotorSrv::Request> req,
    std::shared_ptr<hal::srv::HalArmmotorSrv::Response> res)
{
    // 当前服务暂时作为预留接口。
    // 目前真正的关节控制入口是：
    // hal/manipulator/joint_cmd
    //
    // 后续如果 HalArmmotorSrv 中包含 motor_id / target_position / mode，
    // 可以在这里根据 motor_id_to_joint_index_ 定位到具体关节。
    (void)req;

    res->success = true;
    res->message = "ok";
}

void ManipulatorLifecycleNode::timer_callback()
{
    auto now = this->now();

    bool is_active =
        (this->get_current_state().id() == lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE);

    // ============================================================
    // 1. 状态查询请求
    //
    // 注意：
    // manipulator_driver 不再直接 write can0。
    // 这里只是把 CAN 请求发布到 /hal/can_tx。
    // 真正的 can0 写入由 can_manager 统一排队、限频、发送。
    //
    // 0x08：每 500ms 查询一次本臂所有电机位置
    // 0x0A：每 2s 查询一次本臂所有电机错误状态
    // 0x31：每 2s 查询一次本臂所有电机温度
    // ============================================================
    double dt_pos = (now - last_position_query_time_).seconds();
    double dt_err = (now - last_error_query_time_).seconds();
    double dt_temp = (now - last_temp_query_time_).seconds();

    if (dt_pos >= position_query_period_sec_) {
        for (uint32_t id : expected_motor_ids_) {
            std::array<uint8_t, 8> data{};
            data[0] = 0x08;

            publish_can_frame(
                id,
                1,
                data,
                2,
                "query_position");

            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        last_position_query_time_ = now;
    }

    if (dt_err >= error_query_period_sec_) {
        for (uint32_t id : expected_motor_ids_) {
            std::array<uint8_t, 8> data{};
            data[0] = 0x0A;

            publish_can_frame(
                id,
                1,
                data,
                3,
                "query_error");

            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        last_error_query_time_ = now;
    }

    if (dt_temp >= temp_query_period_sec_) {
        for (uint32_t id : expected_motor_ids_) {
            std::array<uint8_t, 8> data{};
            data[0] = 0x31;

            publish_can_frame(
                id,
                1,
                data,
                4,
                "query_temp");

            std::this_thread::sleep_for(std::chrono::microseconds(100));
        }

        last_temp_query_time_ = now;
    }

    // ============================================================
    // 2. 每个电机 HAL 通信状态判断
    //
    // latest_motor_rx_time_ 不再由 timer_callback 里的 read_frame 更新，
    // 而是由 can_rx_callback() 接收到 /hal/can_rx 后更新。
    // ============================================================
    bool any_motor_ok = false;

    for (uint32_t id : expected_motor_ids_) {
        auto it = latest_motor_rx_time_.find(id);

        if (it == latest_motor_rx_time_.end()) {
            latest_motor_comm_error_[id] = 1;
            continue;
        }

        double dt = (now - it->second).seconds();

        if (dt > comm_timeout_sec_) {
            latest_motor_comm_error_[id] = 1;
        } else {
            latest_motor_comm_error_[id] = 0;
            any_motor_ok = true;
        }
    }

    communication_ok_ = any_motor_ok;

    // ============================================================
    // 3. 安全监控：通信超时
    //
    // last_rx_time_ 由 can_rx_callback() 更新。
    // 如果 active 后长时间没有收到本臂任意电机反馈，则触发 fault stop。
    // ============================================================
    if (is_active && fault_stop_on_comm_loss_) {
        double dt = (now - last_rx_time_).seconds();

        if (dt > comm_timeout_sec_) {
            RCLCPP_ERROR_THROTTLE(
                get_logger(),
                *get_clock(),
                2000,
                "❌ Communication lost -> fault stop");

            fault_stop_requested_ = true;
            control_enabled_ = false;
        }
    }

    // ============================================================
    // 4. 故障 -> 自动退出 activate
    // ============================================================
    if (fault_stop_requested_ && is_active) {
        auto msg_fault = std_msgs::msg::Bool();
        msg_fault.data = true;

        if (fault_pub_) {
            fault_pub_->publish(msg_fault);
        }

        this->deactivate();
        return;
    }

    // ============================================================
    // 5. active 状态：发布 0x44 位置控制请求
    //
    // 这里也不再直接 write can0。
    // 只发布到 /hal/can_tx。
    // can_manager 会对同一 can_id 的 0x44 控制帧只保留最新目标。
    //
    // 发送格式：
    // data[0] = 0x44
    // data[1-4] = 目标位置，小端序
    // ============================================================
    if (is_active && control_enabled_ && !fault_stop_requested_) {
        if (target_joint_position_.size() < expected_motor_ids_.size()) {
            RCLCPP_ERROR(
                get_logger(),
                "target_joint_position_ size (%zu) is smaller than motor count (%zu).",
                target_joint_position_.size(),
                expected_motor_ids_.size());
            return;
        }

        const double ratio = 101.0;

        for (size_t i = 0; i < expected_motor_ids_.size(); ++i) {
            uint32_t id = expected_motor_ids_[i];

            double target_rad = target_joint_position_[i];
            double target_deg = target_rad * 180.0 / M_PI;

            int32_t val =
                static_cast<int32_t>((target_deg / 360.0) * ratio * 65536.0);

            uint32_t u_val = static_cast<uint32_t>(val);

            std::array<uint8_t, 8> data{};
            data[0] = 0x44;
            data[1] = (u_val >> 0) & 0xFF;
            data[2] = (u_val >> 8) & 0xFF;
            data[3] = (u_val >> 16) & 0xFF;
            data[4] = (u_val >> 24) & 0xFF;

            publish_can_frame(
                id,
                5,
                data,
                1,
                "control");

            std::this_thread::sleep_for(std::chrono::microseconds(100));

            // 只打印一次，避免每 20ms 刷屏。
            if (!send_position_log_printed_) {
                RCLCPP_INFO(
                    get_logger(),
                    "✅ Request motor %u (%s): %.2f deg -> 44 %02X %02X %02X %02X",
                    id,
                    (i < joint_names_.size() ? joint_names_[i].c_str() : "unknown_joint"),
                    target_deg,
                    data[1],
                    data[2],
                    data[3],
                    data[4]);
            }
        }

        if (!send_position_log_printed_) {
            send_position_log_printed_ = true;
        }
    }

    // ============================================================
    // 6. 状态上传
    // ============================================================
    if (is_active) {
        publish_joint_states();
        publish_armmotor_state();
    }
}

bool ManipulatorLifecycleNode::process_rx_frame(const hal::msg::CanFrameManipulator& frame)
{
    if (!is_my_motor_id(frame.can_id)) {
        return false;
    }

    // 根据 CAN ID 找到该电机对应的关节下标。
    // 例如左臂：
    // can_id=1 -> idx=0
    // can_id=3 -> idx=1
    // can_id=5 -> idx=2
    // can_id=7 -> idx=3
    // can_id=9 -> idx=4
    auto it = motor_id_to_joint_index_.find(frame.can_id);
    if (it == motor_id_to_joint_index_.end()) {
        RCLCPP_WARN(
            get_logger(),
            "Received frame from motor %u, but no joint index mapping found.",
            frame.can_id);
        return false;
    }

    size_t idx = it->second;
    latest_motor_rx_time_[frame.can_id] = this->now();
    latest_motor_comm_error_[frame.can_id] = 0;

    if (idx >= latest_joint_position_.size()) {
        RCLCPP_ERROR(
            get_logger(),
            "Joint index %zu out of range. latest_joint_position_ size=%zu.",
            idx,
            latest_joint_position_.size());
        return false;
    }

    // ============================================================
    // 1. 原 0x08 位置反馈：5 字节
    //
    // data[0] = 0x08
    // data[1-4] = position

    // ============================================================
    if (frame.dlc == 5 && frame.data[0] == 0x08)
    {
        uint32_t raw_u = 0;
        raw_u |= (uint32_t)(uint8_t)frame.data[1] << 0;
        raw_u |= (uint32_t)(uint8_t)frame.data[2] << 8;
        raw_u |= (uint32_t)(uint8_t)frame.data[3] << 16;
        raw_u |= (uint32_t)(uint8_t)frame.data[4] << 24;

        int32_t raw = static_cast<int32_t>(raw_u);

        const double ratio = 101.0;
        double deg = (raw / 65536.0 / ratio) * 360.0;
        double rad = deg * M_PI / 180.0;

        latest_joint_position_[idx] = rad;
        latest_motor_position_[frame.can_id] = static_cast<int16_t>(deg);

        motor_ready_map_[frame.can_id] = true;

        // 每个电机初始位置只打印一次，避免刷屏。
        if (!initial_pose_printed_map_[frame.can_id]) {
            RCLCPP_INFO(
                get_logger(),
                " Motor %u (%s): 初始位置 = %.2f deg (%.3f rad)",
                frame.can_id,
                (idx < joint_names_.size() ? joint_names_[idx].c_str() : "unknown_joint"),
                deg,
                rad);

            initial_pose_printed_map_[frame.can_id] = true;
        }

        update_initial_pose_completion();
        return true;
    }

    // ============================================================
    // 2. 新 0x44 返回：8 字节，不带命令字
    //
    // data[0-1] = 电流 int16
    // data[2-3] = 速度 int16
    // data[4-7] = 位置 int32
    //
    // 多电机修改：
    // 电流、速度、位置分别写入对应关节 idx。
    // ============================================================
    if (frame.dlc == 8)
    {
        int16_t current = static_cast<int16_t>(
            ((uint16_t)(uint8_t)frame.data[1] << 8) |
            ((uint16_t)(uint8_t)frame.data[0] << 0)
        );

        int16_t speed = static_cast<int16_t>(
            ((uint16_t)(uint8_t)frame.data[3] << 8) |
            ((uint16_t)(uint8_t)frame.data[2] << 0)
        );

        uint32_t raw_position_u = 0;
        raw_position_u |= (uint32_t)(uint8_t)frame.data[4] << 0;
        raw_position_u |= (uint32_t)(uint8_t)frame.data[5] << 8;
        raw_position_u |= (uint32_t)(uint8_t)frame.data[6] << 16;
        raw_position_u |= (uint32_t)(uint8_t)frame.data[7] << 24;

        int32_t raw_position = static_cast<int32_t>(raw_position_u);

        const double ratio = 101.0;
        double deg = (raw_position / 65536.0 / ratio) * 360.0;
        double rad = deg * M_PI / 180.0;

        latest_joint_position_[idx] = rad;
        latest_joint_velocity_[idx] = static_cast<double>(speed);
        latest_joint_effort_[idx] = static_cast<double>(current);

        latest_motor_current_[frame.can_id] = current;
        latest_motor_speed_[frame.can_id] = speed;
        latest_motor_position_[frame.can_id] = static_cast<int16_t>(deg);

        motor_ready_map_[frame.can_id] = true;
        update_initial_pose_completion();

        return true;
    }

    // ============================================================
    // 3. 0x0A 错误状态反馈：5 字节
    //
    // data[0] = 0x0A
    // data[1-4] = error uint32
    //
    // 原始错误状态：
    // bit0  软件错误
    // bit1  过压
    // bit2  欠压
    // bit4  启动错误
    // bit5  速度反馈错误
    // bit6  过流
    // bit16 编码器通讯错误
    // bit17 电机温度过高
    // bit18 电路板温度过高
    // ============================================================
    if (frame.dlc == 5 && frame.data[0] == 0x0A)
    {
        uint32_t err = 0;
        err |= (uint32_t)(uint8_t)frame.data[1] << 0;
        err |= (uint32_t)(uint8_t)frame.data[2] << 8;
        err |= (uint32_t)(uint8_t)frame.data[3] << 16;
        err |= (uint32_t)(uint8_t)frame.data[4] << 24;

        latest_motor_error_[frame.can_id] = err;

        return true;
    }

    // ============================================================
    // 4. 0x31 温度反馈：5 字节
    //
    // 十进制 49 = 0x31
    // data[0] = 0x31
    // data[1-4] = temperature int32
    // 单位：摄氏度
    // ============================================================
    if (frame.dlc == 5 && frame.data[0] == 0x31)
    {
        uint32_t temp_u = 0;
        temp_u |= (uint32_t)(uint8_t)frame.data[1] << 0;
        temp_u |= (uint32_t)(uint8_t)frame.data[2] << 8;
        temp_u |= (uint32_t)(uint8_t)frame.data[3] << 16;
        temp_u |= (uint32_t)(uint8_t)frame.data[4] << 24;

        int32_t temp = static_cast<int32_t>(temp_u);

        if (temp < 0) {
            temp = 0;
        }

        if (temp > 65535) {
            temp = 65535;
        }

        latest_motor_temp_[frame.can_id] = static_cast<uint16_t>(temp);

        return true;
    }

    return false;
}

void ManipulatorLifecycleNode::can_rx_callback(
    const hal::msg::CanFrameManipulator::SharedPtr msg)
{
    if (!msg) {
        return;
    }

    if (!is_my_motor_id(msg->can_id)) {
        return;
    }

    hal::msg::CanFrameManipulator frame;
    frame.stamp = msg->stamp;
    frame.can_id = msg->can_id;
    frame.dlc = msg->dlc;
    frame.priority = msg->priority;
    frame.source = msg->source;
    frame.frame_type = msg->frame_type;

    for (size_t i = 0; i < 8; ++i) {
        frame.data[i] = msg->data[i];
    }

    if (process_rx_frame(frame)) {
        last_rx_time_ = this->now();
        communication_ok_ = true;
        communication_lost_latched_ = false;
    }
}

void ManipulatorLifecycleNode::publish_joint_states()
{
    if (!joint_state_pub_) {
        return;
    }

    if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) {
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

void ManipulatorLifecycleNode::publish_armmotor_state()
{
    if (!armmotor_state_pub_ || !data_upload_enabled_) {
        return;
    }

    if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) {
        return;
    }

    hal::msg::HalArmmotor msg;

    msg.timestamp = this->now().nanoseconds();
    const size_t motor_count = expected_motor_ids_.size();

    msg.motor_current.assign(motor_count, 0);
    msg.motor_speed.assign(motor_count, 0);
    msg.motor_position.assign(motor_count, 0);
    msg.motor_temp.assign(motor_count, 0);
    msg.motor_error.assign(motor_count, 0);
    msg.hal_comm_error.assign(motor_count, 1);

    for (size_t i = 0; i < expected_motor_ids_.size(); ++i) {
        uint32_t id = expected_motor_ids_[i];

        msg.motor_current[i] = latest_motor_current_[id];
        msg.motor_speed[i] = latest_motor_speed_[id];
        msg.motor_position[i] = latest_motor_position_[id];
        msg.motor_temp[i] = latest_motor_temp_[id];

        uint32_t err = latest_motor_error_[id];
        uint8_t compact_err = 0;

        // ========================================================
        // HalArmmotor.msg 中 motor_error 是 byte[]
        // 但是电机协议返回的是 int32_t 错误位。
        //
        // 这里将 int32_t 错误状态压缩成 8 bit：
        //
        // 上传 bit0：软件错误        <- 原始 bit0
        // 上传 bit1：过压            <- 原始 bit1
        // 上传 bit2：欠压            <- 原始 bit2
        // 上传 bit3：启动错误        <- 原始 bit4
        // 上传 bit4：速度反馈错误    <- 原始 bit5
        // 上传 bit5：过流            <- 原始 bit6
        // 上传 bit6：编码器通讯错误  <- 原始 bit16
        // 上传 bit7：温度过高        <- 原始 bit17 或 bit18
        // ========================================================
        if (err & (1u << 0)) {
            compact_err |= (1u << 0);
        }

        if (err & (1u << 1)) {
            compact_err |= (1u << 1);
        }

        if (err & (1u << 2)) {
            compact_err |= (1u << 2);
        }

        if (err & (1u << 4)) {
            compact_err |= (1u << 3);
        }

        if (err & (1u << 5)) {
            compact_err |= (1u << 4);
        }

        if (err & (1u << 6)) {
            compact_err |= (1u << 5);
        }

        if (err & (1u << 16)) {
            compact_err |= (1u << 6);
        }

        if ((err & (1u << 17)) || (err & (1u << 18))) {
            compact_err |= (1u << 7);
        }

        msg.motor_error[i] = compact_err;

        // ========================================================
        // 新增：上传 HAL 与该电机之间的通信状态
        //
        // 注意：
        // 这不是电机自身错误，而是 HAL 层是否还能收到该电机反馈。
        // ========================================================
        auto comm_it = latest_motor_comm_error_.find(id);
        if (comm_it != latest_motor_comm_error_.end()) {
            msg.hal_comm_error[i] = comm_it->second;
        } else {
            msg.hal_comm_error[i] = 1;
        }
    }

    armmotor_state_pub_->publish(msg);
}

void ManipulatorLifecycleNode::publish_end_effector_pose()
{
    // 当前 HAL 层暂不计算末端位姿。
    // 后续可以由 BSP / 运动学节点根据 joint_states 计算并发布。
}

void ManipulatorLifecycleNode::publish_status(const std::string& text)
{
    if (!status_pub_) {
        return;
    }

    if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) {
        return;
    }

    std_msgs::msg::String msg;
    msg.data = text;

    status_pub_->publish(msg);
}

}  // namespace uvms_hal_manipulator

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);

    auto node = std::make_shared<uvms_hal_manipulator::ManipulatorLifecycleNode>();

    rclcpp::executors::SingleThreadedExecutor exec;
    exec.add_node(node->get_node_base_interface());
    exec.spin();

    rclcpp::shutdown();

    return 0;
}