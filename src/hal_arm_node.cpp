#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <cstring>  // <--- 新增这一行！
#include <errno.h>
#include <fcntl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

#include "geometry_msgs/msg/pose_stamped.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "rclcpp/rclcpp.hpp"
#include "rclcpp/executors/single_threaded_executor.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "rclcpp_lifecycle/lifecycle_publisher.hpp"
#include "sensor_msgs/msg/joint_state.hpp"
#include "std_msgs/msg/string.hpp"
#include "std_srvs/srv/set_bool.hpp"
#include "trajectory_msgs/msg/joint_trajectory_point.hpp"

namespace uvms_hal_manipulator
{

// =========================
// protocol_parser.hpp
// =========================
constexpr uint8_t APP_MSG_ID_ARMCABIN_MOTOR_DATA = 0x09;
constexpr uint8_t APP_MSG_ID_ARM_MOTOR_DATA      = 0x0A;
constexpr uint8_t APP_MSG_ID_ARM_CONTROLLER_DATA = 0x0B;
constexpr uint8_t APP_MSG_ID_ARM_CONTROL_CMD     = 0x10;

constexpr uint32_t CAN_ID_ARMCABIN_MOTOR_DATA = 0x09;
constexpr uint32_t CAN_ID_ARM_MOTOR_DATA      = 0x0A;
constexpr uint32_t CAN_ID_ARM_CONTROLLER_DATA = 0x0B;
constexpr uint32_t CAN_ID_ARM_CONTROL_CMD     = 0x10;

enum class ArmControlCommand : uint8_t
{
    CABIN_OPEN  = 0x01,
    CABIN_CLOSE = 0x02,
    ARM_EXTEND  = 0x03,
    ARM_RETRACT = 0x04
};

struct CanFrame
{
    uint32_t can_id{0};
    uint8_t dlc{0};
    std::array<uint8_t, 8> data{};
};

struct ArmCabinMotorState
{
    std::array<int16_t, 2> current{};
    std::array<int16_t, 2> speed{};
    std::array<int16_t, 2> position{};
    std::array<uint16_t, 2> temperature{};
    std::array<uint8_t, 2> error{};
};

struct ArmMotorState
{
    std::array<int16_t, 10> current{};
    std::array<int16_t, 10> speed{};
    std::array<int16_t, 10> position{};
    std::array<uint16_t, 10> temperature{};
    std::array<uint8_t, 10> error{};
};

struct ArmControllerState
{
    std::array<int16_t, 12> current{};
    std::array<int16_t, 12> speed{};
    std::array<int16_t, 12> position{};
    std::array<uint16_t, 12> temperature{};
    std::array<uint8_t, 12> error{};
};

enum class CompleteMessageType : uint8_t
{
    NONE = 0,
    ARMCABIN_MOTOR,
    ARM_MOTOR,
    ARM_CONTROLLER
};

struct CompleteMessage
{
    CompleteMessageType type{CompleteMessageType::NONE};
};

struct FragmentBuffer
{
    uint8_t app_msg_id{0};
    uint8_t total_frames{0};
    std::map<uint8_t, std::vector<uint8_t>> fragments;
};

class ProtocolParser
{
public:
    ProtocolParser() = default;
    ~ProtocolParser() = default;

    CanFrame pack_arm_control_command(ArmControlCommand cmd) const;
    std::optional<CompleteMessage> process_can_frame(const CanFrame& frame);

    bool get_armcabin_motor_state(ArmCabinMotorState& state) const;
    bool get_arm_motor_state(ArmMotorState& state) const;
    bool get_arm_controller_state(ArmControllerState& state) const;

private:
    std::optional<CompleteMessage> process_fragment(
        uint8_t app_msg_id,
        uint8_t total_frames,
        uint8_t frame_index,
        const std::vector<uint8_t>& fragment_payload);

    bool is_complete(const FragmentBuffer& buffer) const;
    std::vector<uint8_t> assemble_payload(const FragmentBuffer& buffer) const;

    bool parse_armcabin_motor_payload(const std::vector<uint8_t>& payload, ArmCabinMotorState& state) const;
    bool parse_arm_motor_payload(const std::vector<uint8_t>& payload, ArmMotorState& state) const;
    bool parse_arm_controller_payload(const std::vector<uint8_t>& payload, ArmControllerState& state) const;

    int16_t read_int16_le(const std::vector<uint8_t>& data, size_t offset) const;
    uint16_t read_uint16_le(const std::vector<uint8_t>& data, size_t offset) const;

private:
    std::map<uint8_t, FragmentBuffer> fragment_buffers_;
    ArmCabinMotorState armcabin_motor_state_{};
    ArmMotorState arm_motor_state_{};
    ArmControllerState arm_controller_state_{};
    bool has_armcabin_motor_state_{false};
    bool has_arm_motor_state_{false};
    bool has_arm_controller_state_{false};
};

// =========================
// can_driver.hpp
// =========================
class CanDriver
{
public:
    CanDriver();
    ~CanDriver();

    CanDriver(const CanDriver&) = delete;
    CanDriver& operator=(const CanDriver&) = delete;

    bool open(const std::string& if_name);
    void close();
    bool is_open() const;
    bool write_frame(const CanFrame& frame);
    bool read_frame(CanFrame& frame);
    void flush();

private:
    int socket_fd_;
    std::string if_name_;
};

// =========================
// safety_manager.hpp
// =========================
struct JointLimitConfig
{
    std::vector<double> position_min;
    std::vector<double> position_max;
    std::vector<double> max_velocity;
};

struct MotorSafetyConfig
{
    double max_current{0.0};
    double max_temperature{0.0};
    double communication_timeout_sec{0.2};
};

struct SafetyCheckResult
{
    bool ok{true};
    std::string reason;
};

class SafetyManager
{
public:
    SafetyManager() = default;
    ~SafetyManager() = default;

    void set_joint_limit_config(const JointLimitConfig& config);
    void set_motor_safety_config(const MotorSafetyConfig& config);

    void set_estop(bool estop);
    bool is_estop() const;

    SafetyCheckResult validate_joint_command(
        const std::vector<double>& positions,
        const std::vector<double>& velocities) const;

    std::vector<double> clamp_velocity(
        const std::vector<double>& velocities) const;

    SafetyCheckResult check_motor_overload(
        const std::vector<double>& currents,
        const std::vector<double>& temperatures) const;

    SafetyCheckResult check_communication_timeout(
        double elapsed_sec) const;

private:
    bool check_size_match(
        const std::vector<double>& positions,
        const std::vector<double>& velocities) const;

private:
    JointLimitConfig joint_limit_config_;
    MotorSafetyConfig motor_safety_config_;
    bool estop_{false};
};

// =========================
// manipulator_lifecycle_node.hpp
// =========================
class ManipulatorLifecycleNode : public rclcpp_lifecycle::LifecycleNode
{
public:
    explicit ManipulatorLifecycleNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions());
    ~ManipulatorLifecycleNode() override = default;

protected:
    using CallbackReturn =
        rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

    CallbackReturn on_configure(const rclcpp_lifecycle::State& state) override;
    CallbackReturn on_activate(const rclcpp_lifecycle::State& state) override;
    CallbackReturn on_deactivate(const rclcpp_lifecycle::State& state) override;
    CallbackReturn on_cleanup(const rclcpp_lifecycle::State& state) override;
    CallbackReturn on_shutdown(const rclcpp_lifecycle::State& state) override;
    CallbackReturn on_error(const rclcpp_lifecycle::State& state) override;

private:
    void joint_cmd_callback(
        const trajectory_msgs::msg::JointTrajectoryPoint::SharedPtr msg);

    void emergency_stop_callback(
        const std::shared_ptr<std_srvs::srv::SetBool::Request> request,
        std::shared_ptr<std_srvs::srv::SetBool::Response> response);

    void timer_callback();

    void declare_and_load_parameters();
    bool init_safety_config();
    bool init_can_driver();
    void reset_runtime_state();

    bool process_rx_frame(const CanFrame& frame);
    void publish_joint_states();
    void publish_end_effector_pose();
    void publish_status(const std::string& text);

    std::vector<double> int16_array_to_double_vector_2(
        const std::array<int16_t, 2>& arr) const;

    std::vector<double> int16_array_to_double_vector_10(
        const std::array<int16_t, 10>& arr) const;

    std::vector<double> uint16_array_to_double_vector_10(
        const std::array<uint16_t, 10>& arr) const;

private:
    std::string arm_name_;
    std::string can_interface_;
    std::string base_frame_;
    std::string ee_frame_;
    double publish_rate_hz_{50.0};
    bool debug_mode_{false};

    std::vector<std::string> joint_names_;
    std::vector<double> joint_pos_min_;
    std::vector<double> joint_pos_max_;
    std::vector<double> joint_vel_max_;

    double max_current_{500.0};
    double max_temperature_{100.0};
    double comm_timeout_sec_{0.2};

    rclcpp::Subscription<trajectory_msgs::msg::JointTrajectoryPoint>::SharedPtr joint_cmd_sub_;
    rclcpp::Service<std_srvs::srv::SetBool>::SharedPtr emergency_stop_srv_;

    rclcpp_lifecycle::LifecyclePublisher<sensor_msgs::msg::JointState>::SharedPtr joint_state_pub_;
    rclcpp_lifecycle::LifecyclePublisher<geometry_msgs::msg::PoseStamped>::SharedPtr ee_pose_pub_;
    rclcpp_lifecycle::LifecyclePublisher<std_msgs::msg::String>::SharedPtr status_pub_;

    rclcpp::TimerBase::SharedPtr timer_;

    ProtocolParser protocol_parser_;
    CanDriver can_driver_;
    SafetyManager safety_manager_;

    rclcpp::Time last_rx_time_;
    bool communication_ok_{false};

    std::vector<double> latest_joint_position_;
    std::vector<double> latest_joint_velocity_;
    std::vector<double> latest_joint_effort_;

    ArmCabinMotorState latest_armcabin_motor_state_{};
    ArmMotorState latest_arm_motor_state_{};
    ArmControllerState latest_arm_controller_state_{};
};

// =========================
// can_driver.cpp
// =========================
CanDriver::CanDriver()
: socket_fd_(-1)
{
}

CanDriver::~CanDriver()
{
    close();
}

bool CanDriver::open(const std::string& if_name)
{
    close();

    if_name_ = if_name;
    socket_fd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socket_fd_ < 0) {
        std::cerr << "[CanDriver] Failed to create CAN socket." << std::endl;
        return false;
    }

    struct ifreq ifr {};
    std::strncpy(ifr.ifr_name, if_name.c_str(), IFNAMSIZ - 1);

    if (ioctl(socket_fd_, SIOCGIFINDEX, &ifr) < 0) {
        std::cerr << "[CanDriver] Failed to get interface index for " << if_name << std::endl;
        close();
        return false;
    }

    struct sockaddr_can addr {};
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(socket_fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "[CanDriver] Failed to bind CAN socket to " << if_name << std::endl;
        close();
        return false;
    }

    int flags = fcntl(socket_fd_, F_GETFL, 0);
    if (flags < 0) {
        std::cerr << "[CanDriver] Failed to get socket flags." << std::endl;
        close();
        return false;
    }

    if (fcntl(socket_fd_, F_SETFL, flags | O_NONBLOCK) < 0) {
        std::cerr << "[CanDriver] Failed to set CAN socket non-blocking." << std::endl;
        close();
        return false;
    }

    return true;
}

void CanDriver::close()
{
    if (socket_fd_ >= 0) {
        ::close(socket_fd_);
        socket_fd_ = -1;
    }
}

bool CanDriver::is_open() const
{
    return socket_fd_ >= 0;
}

bool CanDriver::write_frame(const CanFrame& frame)
{
    if (!is_open()) {
        return false;
    }

    struct can_frame raw_frame {};
    raw_frame.can_id = frame.can_id;
    // 将 raw_frame.len = frame.dlc; 修改为：
    raw_frame.can_dlc = frame.dlc;

    for (uint8_t i = 0; i < frame.dlc && i < 8; ++i) {
        raw_frame.data[i] = frame.data[i];
    }

    const ssize_t nbytes = ::write(socket_fd_, &raw_frame, sizeof(raw_frame));
    if (nbytes != static_cast<ssize_t>(sizeof(raw_frame))) {
        std::cerr << "[CanDriver] Failed to write CAN frame." << std::endl;
        return false;
    }

    return true;
}

bool CanDriver::read_frame(CanFrame& frame)
{
    if (!is_open()) {
        return false;
    }

    struct can_frame raw_frame {};
    const ssize_t nbytes = ::read(socket_fd_, &raw_frame, sizeof(raw_frame));

    if (nbytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return false;
        }
        std::cerr << "[CanDriver] Failed to read CAN frame: "
                  << std::strerror(errno) << std::endl;
        return false;
    }

    if (nbytes == 0) {
        return false;
    }

    if (nbytes < static_cast<ssize_t>(sizeof(struct can_frame))) {
        std::cerr << "[CanDriver] Incomplete CAN frame received." << std::endl;
        return false;
    }

    frame.can_id = raw_frame.can_id & CAN_EFF_MASK;
    // 将 frame.dlc = raw_frame.len; 修改为：
    frame.dlc = raw_frame.can_dlc;
    frame.data.fill(0);

    for (uint8_t i = 0; i < frame.dlc && i < 8; ++i) {
        frame.data[i] = raw_frame.data[i];
    }

    return true;
}

void CanDriver::flush()
{
    if (!is_open()) {
        return;
    }

    CanFrame frame;
    while (read_frame(frame)) {
    }
}

// =========================
// safety_manager.cpp
// =========================
void SafetyManager::set_joint_limit_config(const JointLimitConfig& config)
{
    joint_limit_config_ = config;
}

void SafetyManager::set_motor_safety_config(const MotorSafetyConfig& config)
{
    motor_safety_config_ = config;
}

void SafetyManager::set_estop(bool estop)
{
    estop_ = estop;
}

bool SafetyManager::is_estop() const
{
    return estop_;
}

SafetyCheckResult SafetyManager::validate_joint_command(
    const std::vector<double>& positions,
    const std::vector<double>& velocities) const
{
    SafetyCheckResult result;

    if (estop_) {
        result.ok = false;
        result.reason = "Emergency stop is active.";
        return result;
    }

    if (!check_size_match(positions, velocities)) {
        result.ok = false;
        result.reason = "Joint command size mismatch.";
        return result;
    }

    const size_t joint_num = positions.size();
    if (joint_limit_config_.position_min.size() != joint_num ||
        joint_limit_config_.position_max.size() != joint_num ||
        joint_limit_config_.max_velocity.size() != joint_num) {
        result.ok = false;
        result.reason = "Joint limit config size mismatch.";
        return result;
    }

    for (size_t i = 0; i < joint_num; ++i) {
        if (positions[i] < joint_limit_config_.position_min[i] ||
            positions[i] > joint_limit_config_.position_max[i]) {
            std::ostringstream oss;
            oss << "Joint position out of range at index " << i;
            result.ok = false;
            result.reason = oss.str();
            return result;
        }

        if (std::fabs(velocities[i]) > joint_limit_config_.max_velocity[i]) {
            std::ostringstream oss;
            oss << "Joint velocity exceeds limit at index " << i;
            result.ok = false;
            result.reason = oss.str();
            return result;
        }
    }

    result.ok = true;
    result.reason = "Joint command valid.";
    return result;
}

std::vector<double> SafetyManager::clamp_velocity(
    const std::vector<double>& velocities) const
{
    std::vector<double> clamped = velocities;
    const size_t n = std::min(clamped.size(), joint_limit_config_.max_velocity.size());

    for (size_t i = 0; i < n; ++i) {
        const double vmax = joint_limit_config_.max_velocity[i];
        if (clamped[i] > vmax) {
            clamped[i] = vmax;
        } else if (clamped[i] < -vmax) {
            clamped[i] = -vmax;
        }
    }

    return clamped;
}

SafetyCheckResult SafetyManager::check_motor_overload(
    const std::vector<double>& currents,
    const std::vector<double>& temperatures) const
{
    SafetyCheckResult result;
    const size_t n = std::min(currents.size(), temperatures.size());

    if (n == 0) {
        result.ok = false;
        result.reason = "Empty motor status data.";
        return result;
    }

    for (size_t i = 0; i < n; ++i) {
        if (currents[i] > motor_safety_config_.max_current) {
            std::ostringstream oss;
            oss << "Motor current overload at index " << i;
            result.ok = false;
            result.reason = oss.str();
            return result;
        }

        if (temperatures[i] > motor_safety_config_.max_temperature) {
            std::ostringstream oss;
            oss << "Motor temperature overload at index " << i;
            result.ok = false;
            result.reason = oss.str();
            return result;
        }
    }

    result.ok = true;
    result.reason = "Motor status normal.";
    return result;
}

SafetyCheckResult SafetyManager::check_communication_timeout(
    double elapsed_sec) const
{
    SafetyCheckResult result;
    if (elapsed_sec > motor_safety_config_.communication_timeout_sec) {
        result.ok = false;
        result.reason = "Communication timeout.";
        return result;
    }

    result.ok = true;
    result.reason = "Communication normal.";
    return result;
}

bool SafetyManager::check_size_match(
    const std::vector<double>& positions,
    const std::vector<double>& velocities) const
{
    return !positions.empty() &&
           !velocities.empty() &&
           positions.size() == velocities.size();
}

// =========================
// protocol_parser.cpp
// =========================
CanFrame ProtocolParser::pack_arm_control_command(ArmControlCommand cmd) const
{
    CanFrame frame;
    frame.can_id = CAN_ID_ARM_CONTROL_CMD;
    frame.dlc = 8;
    frame.data.fill(0);
    frame.data[0] = APP_MSG_ID_ARM_CONTROL_CMD;
    frame.data[1] = static_cast<uint8_t>(cmd);
    return frame;
}

std::optional<CompleteMessage> ProtocolParser::process_can_frame(const CanFrame& frame)
{
    if (frame.dlc < 3) {
        return std::nullopt;
    }

    const uint8_t app_msg_id = frame.data[0];
    const uint8_t total_frames = frame.data[1];
    const uint8_t frame_index = frame.data[2];

    std::vector<uint8_t> fragment_payload;
    for (uint8_t i = 3; i < frame.dlc; ++i) {
        fragment_payload.push_back(frame.data[i]);
    }

    return process_fragment(app_msg_id, total_frames, frame_index, fragment_payload);
}

std::optional<CompleteMessage> ProtocolParser::process_fragment(
    uint8_t app_msg_id,
    uint8_t total_frames,
    uint8_t frame_index,
    const std::vector<uint8_t>& fragment_payload)
{
    if (total_frames == 0) {
        return std::nullopt;
    }

    auto& buffer = fragment_buffers_[app_msg_id];
    if (buffer.fragments.empty()) {
        buffer.app_msg_id = app_msg_id;
        buffer.total_frames = total_frames;
    }

    if (buffer.total_frames != total_frames) {
        buffer.fragments.clear();
        buffer.app_msg_id = app_msg_id;
        buffer.total_frames = total_frames;
    }

    buffer.fragments[frame_index] = fragment_payload;

    if (!is_complete(buffer)) {
        return std::nullopt;
    }

    const auto payload = assemble_payload(buffer);
    buffer.fragments.clear();

    CompleteMessage msg;

    if (app_msg_id == APP_MSG_ID_ARMCABIN_MOTOR_DATA) {
        if (parse_armcabin_motor_payload(payload, armcabin_motor_state_)) {
            has_armcabin_motor_state_ = true;
            msg.type = CompleteMessageType::ARMCABIN_MOTOR;
            return msg;
        }
    } else if (app_msg_id == APP_MSG_ID_ARM_MOTOR_DATA) {
        if (parse_arm_motor_payload(payload, arm_motor_state_)) {
            has_arm_motor_state_ = true;
            msg.type = CompleteMessageType::ARM_MOTOR;
            return msg;
        }
    } else if (app_msg_id == APP_MSG_ID_ARM_CONTROLLER_DATA) {
        if (parse_arm_controller_payload(payload, arm_controller_state_)) {
            has_arm_controller_state_ = true;
            msg.type = CompleteMessageType::ARM_CONTROLLER;
            return msg;
        }
    }

    return std::nullopt;
}

bool ProtocolParser::is_complete(const FragmentBuffer& buffer) const
{
    if (buffer.fragments.size() != buffer.total_frames) {
        return false;
    }

    for (uint8_t i = 0; i < buffer.total_frames; ++i) {
        if (buffer.fragments.find(i) == buffer.fragments.end()) {
            return false;
        }
    }

    return true;
}

std::vector<uint8_t> ProtocolParser::assemble_payload(const FragmentBuffer& buffer) const
{
    std::vector<uint8_t> payload;
    for (uint8_t i = 0; i < buffer.total_frames; ++i) {
        const auto& frag = buffer.fragments.at(i);
        payload.insert(payload.end(), frag.begin(), frag.end());
    }
    return payload;
}

bool ProtocolParser::parse_armcabin_motor_payload(
    const std::vector<uint8_t>& payload,
    ArmCabinMotorState& state) const
{
    constexpr size_t expected_size = 4 + 4 + 4 + 4 + 2;
    if (payload.size() < expected_size) {
        return false;
    }

    size_t offset = 0;
    for (size_t i = 0; i < 2; ++i, offset += 2) state.current[i] = read_int16_le(payload, offset);
    for (size_t i = 0; i < 2; ++i, offset += 2) state.speed[i] = read_int16_le(payload, offset);
    for (size_t i = 0; i < 2; ++i, offset += 2) state.position[i] = read_int16_le(payload, offset);
    for (size_t i = 0; i < 2; ++i, offset += 2) state.temperature[i] = read_uint16_le(payload, offset);
    for (size_t i = 0; i < 2; ++i, offset += 1) state.error[i] = payload[offset];

    return true;
}

bool ProtocolParser::parse_arm_motor_payload(
    const std::vector<uint8_t>& payload,
    ArmMotorState& state) const
{
    constexpr size_t expected_size = 20 + 20 + 20 + 20 + 10;
    if (payload.size() < expected_size) {
        return false;
    }

    size_t offset = 0;
    for (size_t i = 0; i < 10; ++i, offset += 2) state.current[i] = read_int16_le(payload, offset);
    for (size_t i = 0; i < 10; ++i, offset += 2) state.speed[i] = read_int16_le(payload, offset);
    for (size_t i = 0; i < 10; ++i, offset += 2) state.position[i] = read_int16_le(payload, offset);
    for (size_t i = 0; i < 10; ++i, offset += 2) state.temperature[i] = read_uint16_le(payload, offset);
    for (size_t i = 0; i < 10; ++i, offset += 1) state.error[i] = payload[offset];

    return true;
}

bool ProtocolParser::parse_arm_controller_payload(
    const std::vector<uint8_t>& payload,
    ArmControllerState& state) const
{
    constexpr size_t expected_size = 24 + 24 + 24 + 24 + 12;
    if (payload.size() < expected_size) {
        return false;
    }

    size_t offset = 0;
    for (size_t i = 0; i < 12; ++i, offset += 2) state.current[i] = read_int16_le(payload, offset);
    for (size_t i = 0; i < 12; ++i, offset += 2) state.speed[i] = read_int16_le(payload, offset);
    for (size_t i = 0; i < 12; ++i, offset += 2) state.position[i] = read_int16_le(payload, offset);
    for (size_t i = 0; i < 12; ++i, offset += 2) state.temperature[i] = read_uint16_le(payload, offset);
    for (size_t i = 0; i < 12; ++i, offset += 1) state.error[i] = payload[offset];

    return true;
}

bool ProtocolParser::get_armcabin_motor_state(ArmCabinMotorState& state) const
{
    if (!has_armcabin_motor_state_) {
        return false;
    }
    state = armcabin_motor_state_;
    return true;
}

bool ProtocolParser::get_arm_motor_state(ArmMotorState& state) const
{
    if (!has_arm_motor_state_) {
        return false;
    }
    state = arm_motor_state_;
    return true;
}

bool ProtocolParser::get_arm_controller_state(ArmControllerState& state) const
{
    if (!has_arm_controller_state_) {
        return false;
    }
    state = arm_controller_state_;
    return true;
}

int16_t ProtocolParser::read_int16_le(const std::vector<uint8_t>& data, size_t offset) const
{
    if (offset + 1 >= data.size()) {
        throw std::out_of_range("read_int16_le out of range");
    }

    return static_cast<int16_t>(
        static_cast<uint16_t>(data[offset]) |
        (static_cast<uint16_t>(data[offset + 1]) << 8));
}

uint16_t ProtocolParser::read_uint16_le(const std::vector<uint8_t>& data, size_t offset) const
{
    if (offset + 1 >= data.size()) {
        throw std::out_of_range("read_uint16_le out of range");
    }

    return static_cast<uint16_t>(data[offset]) |
           (static_cast<uint16_t>(data[offset + 1]) << 8);
}

// =========================
// manipulator_lifecycle_node.cpp
// =========================
ManipulatorLifecycleNode::ManipulatorLifecycleNode(const rclcpp::NodeOptions& options)
: rclcpp_lifecycle::LifecycleNode("manipulator_driver", options),
  arm_name_("arm"),
  can_interface_("can0"),
  base_frame_("base_link"),
  ee_frame_("ee_link"),
  last_rx_time_(0, 0, RCL_ROS_TIME)
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

    joint_cmd_sub_ = create_subscription<trajectory_msgs::msg::JointTrajectoryPoint>(
        "hal/manipulator/joint_cmd",
        rclcpp::QoS(10),
        std::bind(&ManipulatorLifecycleNode::joint_cmd_callback, this, std::placeholders::_1));

    emergency_stop_srv_ = create_service<std_srvs::srv::SetBool>(
        "hal/manipulator/emergency_stop",
        std::bind(
            &ManipulatorLifecycleNode::emergency_stop_callback,
            this,
            std::placeholders::_1,
            std::placeholders::_2));

    joint_state_pub_ =
        create_publisher<sensor_msgs::msg::JointState>("hal/manipulator/joint_states", rclcpp::QoS(10));

    ee_pose_pub_ =
        create_publisher<geometry_msgs::msg::PoseStamped>("hal/manipulator/end_effector_pose", rclcpp::QoS(10));

    status_pub_ =
        create_publisher<std_msgs::msg::String>("hal/manipulator/status", rclcpp::QoS(10));

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

    latest_joint_position_ = positions;
    latest_joint_velocity_ = safety_manager_.clamp_velocity(velocities);

    publish_status("Joint command accepted (placeholder, waiting for detailed CAN motion protocol).");
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

    CanFrame frame;
    bool received = false;

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

    if (!debug_mode_) {
        const double elapsed = (this->now() - last_rx_time_).seconds();
        const auto comm_result = safety_manager_.check_communication_timeout(elapsed);
        if (!comm_result.ok) {
            communication_ok_ = false;
            publish_status(comm_result.reason);
        }
    } else {
        communication_ok_ = true;
    }

    const auto currents = int16_array_to_double_vector_10(latest_arm_motor_state_.current);
    const auto temperatures = uint16_array_to_double_vector_10(latest_arm_motor_state_.temperature);
    const auto overload_result = safety_manager_.check_motor_overload(currents, temperatures);
    if (!overload_result.ok) {
        publish_status(overload_result.reason);
    }

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
    std::vector<double> out(arr.size(), 0.0);
    for (size_t i = 0; i < arr.size(); ++i) {
        out[i] = static_cast<double>(arr[i]);
    }
    return out;
}

std::vector<double> ManipulatorLifecycleNode::int16_array_to_double_vector_10(
    const std::array<int16_t, 10>& arr) const
{
    std::vector<double> out(arr.size(), 0.0);
    for (size_t i = 0; i < arr.size(); ++i) {
        out[i] = static_cast<double>(arr[i]);
    }
    return out;
}

std::vector<double> ManipulatorLifecycleNode::uint16_array_to_double_vector_10(
    const std::array<uint16_t, 10>& arr) const
{
    std::vector<double> out(arr.size(), 0.0);
    for (size_t i = 0; i < arr.size(); ++i) {
        out[i] = static_cast<double>(arr[i]);
    }
    return out;
}

}  // namespace uvms_hal_manipulator

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);

    auto node = std::make_shared<uvms_hal_manipulator::ManipulatorLifecycleNode>();
    rclcpp::executors::SingleThreadedExecutor executor;
    executor.add_node(node->get_node_base_interface());
    executor.spin();

    rclcpp::shutdown();
    return 0;
}
