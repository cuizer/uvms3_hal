#include "manipulator_hal/safety_manager.hpp"

#include <cmath>
#include <sstream>

namespace uvms_hal_manipulator
{

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

}  // namespace uvms_hal_manipulator