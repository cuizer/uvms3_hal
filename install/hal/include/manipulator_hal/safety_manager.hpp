#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace uvms_hal_manipulator
{

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

    // 配置接口
    void set_joint_limit_config(const JointLimitConfig& config);
    void set_motor_safety_config(const MotorSafetyConfig& config);

    // 急停控制
    void set_estop(bool estop);
    bool is_estop() const;

    // 命令检查
    SafetyCheckResult validate_joint_command(
        const std::vector<double>& positions,
        const std::vector<double>& velocities) const;

    // 速度限幅
    std::vector<double> clamp_velocity(
        const std::vector<double>& velocities) const;

    // 状态安全检查
    SafetyCheckResult check_motor_overload(
        const std::vector<double>& currents,
        const std::vector<double>& temperatures) const;

    // 通信超时检查
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

}  // namespace uvms_hal_manipulator