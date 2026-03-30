// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from hal:msg/HalAuxithrusterMsg.idl
// generated code does not contain a copyright notice

#ifndef HAL__MSG__DETAIL__HAL_AUXITHRUSTER_MSG__BUILDER_HPP_
#define HAL__MSG__DETAIL__HAL_AUXITHRUSTER_MSG__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "hal/msg/detail/hal_auxithruster_msg__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace hal
{

namespace msg
{

namespace builder
{

class Init_HalAuxithrusterMsg_fault_status
{
public:
  explicit Init_HalAuxithrusterMsg_fault_status(::hal::msg::HalAuxithrusterMsg & msg)
  : msg_(msg)
  {}
  ::hal::msg::HalAuxithrusterMsg fault_status(::hal::msg::HalAuxithrusterMsg::_fault_status_type arg)
  {
    msg_.fault_status = std::move(arg);
    return std::move(msg_);
  }

private:
  ::hal::msg::HalAuxithrusterMsg msg_;
};

class Init_HalAuxithrusterMsg_esc_status
{
public:
  explicit Init_HalAuxithrusterMsg_esc_status(::hal::msg::HalAuxithrusterMsg & msg)
  : msg_(msg)
  {}
  Init_HalAuxithrusterMsg_fault_status esc_status(::hal::msg::HalAuxithrusterMsg::_esc_status_type arg)
  {
    msg_.esc_status = std::move(arg);
    return Init_HalAuxithrusterMsg_fault_status(msg_);
  }

private:
  ::hal::msg::HalAuxithrusterMsg msg_;
};

class Init_HalAuxithrusterMsg_temp
{
public:
  explicit Init_HalAuxithrusterMsg_temp(::hal::msg::HalAuxithrusterMsg & msg)
  : msg_(msg)
  {}
  Init_HalAuxithrusterMsg_esc_status temp(::hal::msg::HalAuxithrusterMsg::_temp_type arg)
  {
    msg_.temp = std::move(arg);
    return Init_HalAuxithrusterMsg_esc_status(msg_);
  }

private:
  ::hal::msg::HalAuxithrusterMsg msg_;
};

class Init_HalAuxithrusterMsg_voltage
{
public:
  explicit Init_HalAuxithrusterMsg_voltage(::hal::msg::HalAuxithrusterMsg & msg)
  : msg_(msg)
  {}
  Init_HalAuxithrusterMsg_temp voltage(::hal::msg::HalAuxithrusterMsg::_voltage_type arg)
  {
    msg_.voltage = std::move(arg);
    return Init_HalAuxithrusterMsg_temp(msg_);
  }

private:
  ::hal::msg::HalAuxithrusterMsg msg_;
};

class Init_HalAuxithrusterMsg_current
{
public:
  explicit Init_HalAuxithrusterMsg_current(::hal::msg::HalAuxithrusterMsg & msg)
  : msg_(msg)
  {}
  Init_HalAuxithrusterMsg_voltage current(::hal::msg::HalAuxithrusterMsg::_current_type arg)
  {
    msg_.current = std::move(arg);
    return Init_HalAuxithrusterMsg_voltage(msg_);
  }

private:
  ::hal::msg::HalAuxithrusterMsg msg_;
};

class Init_HalAuxithrusterMsg_rpm
{
public:
  Init_HalAuxithrusterMsg_rpm()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_HalAuxithrusterMsg_current rpm(::hal::msg::HalAuxithrusterMsg::_rpm_type arg)
  {
    msg_.rpm = std::move(arg);
    return Init_HalAuxithrusterMsg_current(msg_);
  }

private:
  ::hal::msg::HalAuxithrusterMsg msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::hal::msg::HalAuxithrusterMsg>()
{
  return hal::msg::builder::Init_HalAuxithrusterMsg_rpm();
}

}  // namespace hal

#endif  // HAL__MSG__DETAIL__HAL_AUXITHRUSTER_MSG__BUILDER_HPP_
