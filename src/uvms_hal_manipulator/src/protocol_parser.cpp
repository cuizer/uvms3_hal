#include "uvms_hal_manipulator/protocol_parser.hpp"

#include <stdexcept>

namespace uvms_hal_manipulator
{

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
    if (frame.dlc == 0) {
        return std::nullopt;
    }

    // 控制命令帧一般是发出去的，这里主要处理状态反馈帧
    // 约定状态帧格式：
    // data[0] = app_msg_id
    // data[1] = total_frames
    // data[2] = frame_index
    // data[3..7] = fragment payload

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

    // 若 total_frames 变化，说明开始了一组新消息，重置
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
    // 2组:
    // current(2*2) + speed(2*2) + position(2*2) + temp(2*2) + error(2*1)
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
    // 10组:
    // current(10*2) + speed(10*2) + position(10*2) + temp(10*2) + error(10*1)
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
    // 12组:
    // current(12*2) + speed(12*2) + position(12*2) + temp(12*2) + error(12*1)
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

}  // namespace uvms_hal_manipulator