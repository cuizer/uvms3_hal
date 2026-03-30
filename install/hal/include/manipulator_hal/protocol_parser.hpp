#pragma once

#include <array>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace uvms_hal_manipulator
{

// =========================
// 应用消息号定义
// =========================
constexpr uint8_t APP_MSG_ID_ARMCABIN_MOTOR_DATA = 0x09;   // 机械臂舱电机信息
constexpr uint8_t APP_MSG_ID_ARM_MOTOR_DATA      = 0x0A;   // 机械臂电机信息
constexpr uint8_t APP_MSG_ID_ARM_CONTROLLER_DATA = 0x0B;   // 主手电机信息
constexpr uint8_t APP_MSG_ID_ARM_CONTROL_CMD     = 0x10;   // 机械臂控制指令 (#16)

// 默认 CAN ID 映射（通用假设）
constexpr uint32_t CAN_ID_ARMCABIN_MOTOR_DATA = 0x09;
constexpr uint32_t CAN_ID_ARM_MOTOR_DATA      = 0x0A;
constexpr uint32_t CAN_ID_ARM_CONTROLLER_DATA = 0x0B;
constexpr uint32_t CAN_ID_ARM_CONTROL_CMD     = 0x10;

// =========================
// 控制指令定义
// =========================
enum class ArmControlCommand : uint8_t
{
    CABIN_OPEN  = 0x01,  // 机械臂舱开启
    CABIN_CLOSE = 0x02,  // 机械臂舱关闭
    ARM_EXTEND  = 0x03,  // 机械臂伸出
    ARM_RETRACT = 0x04   // 机械臂回收
};

// =========================
// CAN 帧结构（通用）
// =========================
struct CanFrame
{
    uint32_t can_id{0};
    uint8_t dlc{0};
    std::array<uint8_t, 8> data{};
};

// =========================
// 上层状态结构
// =========================
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

// =========================
// 重组后的完整消息类型
// =========================
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

// =========================
// 分片缓存结构
// =========================
struct FragmentBuffer
{
    uint8_t app_msg_id{0};
    uint8_t total_frames{0};
    std::map<uint8_t, std::vector<uint8_t>> fragments;
};

// =========================
// 协议解析类
// =========================
class ProtocolParser
{
public:
    ProtocolParser() = default;
    ~ProtocolParser() = default;

    // 打包机械臂控制命令（#16）
    CanFrame pack_arm_control_command(ArmControlCommand cmd) const;

    // 输入单帧 CAN 报文，若重组完成则返回完整消息类型
    std::optional<CompleteMessage> process_can_frame(const CanFrame& frame);

    // 获取最近一次解析完成的数据
    bool get_armcabin_motor_state(ArmCabinMotorState& state) const;
    bool get_arm_motor_state(ArmMotorState& state) const;
    bool get_arm_controller_state(ArmControllerState& state) const;

private:
    // 按应用消息号处理分片
    std::optional<CompleteMessage> process_fragment(
        uint8_t app_msg_id,
        uint8_t total_frames,
        uint8_t frame_index,
        const std::vector<uint8_t>& fragment_payload);

    // 检查是否收齐
    bool is_complete(const FragmentBuffer& buffer) const;

    // 拼接完整 payload
    std::vector<uint8_t> assemble_payload(const FragmentBuffer& buffer) const;

    // 解析完整 payload
    bool parse_armcabin_motor_payload(const std::vector<uint8_t>& payload, ArmCabinMotorState& state) const;
    bool parse_arm_motor_payload(const std::vector<uint8_t>& payload, ArmMotorState& state) const;
    bool parse_arm_controller_payload(const std::vector<uint8_t>& payload, ArmControllerState& state) const;

    // 小端序读取工具
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

}  // namespace uvms_hal_manipulator