// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from hal:msg/HalMainthrusterMsg.idl
// generated code does not contain a copyright notice

#ifndef HAL__MSG__DETAIL__HAL_MAINTHRUSTER_MSG__BUILDER_HPP_
#define HAL__MSG__DETAIL__HAL_MAINTHRUSTER_MSG__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "hal/msg/detail/hal_mainthruster_msg__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace hal
{

namespace msg
{

namespace builder
{

class Init_HalMainthrusterMsg_fault_status
{
public:
  explicit Init_HalMainthrusterMsg_fault_status(::hal::msg::HalMainthrusterMsg & msg)
  : msg_(msg)
  {}
  ::hal::msg::HalMainthrusterMsg fault_status(::hal::msg::HalMainthrusterMsg::_fault_status_type arg)
  {
    msg_.fault_status = std::move(arg);
    return std::move(msg_);
  }

private:
  ::hal::msg::HalMainthrusterMsg msg_;
};

class Init_HalMainthrusterMsg_voltage
{
public:
  explicit Init_HalMainthrusterMsg_voltage(::hal::msg::HalMainthrusterMsg & msg)
  : msg_(msg)
  {}
  Init_HalMainthrusterMsg_fault_status voltage(::hal::msg::HalMainthrusterMsg::_voltage_type arg)
  {
    msg_.voltage = std::move(arg);
    return Init_HalMainthrusterMsg_fault_status(msg_);
  }

private:
  ::hal::msg::HalMainthrusterMsg msg_;
};

class Init_HalMainthrusterMsg_current
{
public:
  explicit Init_HalMainthrusterMsg_current(::hal::msg::HalMainthrusterMsg & msg)
  : msg_(msg)
  {}
  Init_HalMainthrusterMsg_voltage current(::hal::msg::HalMainthrusterMsg::_current_type arg)
  {
    msg_.current = std::move(arg);
    return Init_HalMainthrusterMsg_voltage(msg_);
  }

private:
  ::hal::msg::HalMainthrusterMsg msg_;
};

class Init_HalMainthrusterMsg_rpm
{
public:
  Init_HalMainthrusterMsg_rpm()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_HalMainthrusterMsg_current rpm(::hal::msg::HalMainthrusterMsg::_rpm_type arg)
  {
    msg_.rpm = std::move(arg);
    return Init_HalMainthrusterMsg_current(msg_);
  }

private:
  ::hal::msg::HalMainthrusterMsg msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::hal::msg::HalMainthrusterMsg>()
{
  return hal::msg::builder::Init_HalMainthrusterMsg_rpm();
}

}  // namespace hal

#endif  // HAL__MSG__DETAIL__HAL_MAINTHRUSTER_MSG__BUILDER_HPP_
