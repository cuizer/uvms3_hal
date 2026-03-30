// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from hal:msg/HalBatteryMsg.idl
// generated code does not contain a copyright notice

#ifndef HAL__MSG__DETAIL__HAL_BATTERY_MSG__BUILDER_HPP_
#define HAL__MSG__DETAIL__HAL_BATTERY_MSG__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "hal/msg/detail/hal_battery_msg__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace hal
{

namespace msg
{

namespace builder
{

class Init_HalBatteryMsg_switch_state
{
public:
  explicit Init_HalBatteryMsg_switch_state(::hal::msg::HalBatteryMsg & msg)
  : msg_(msg)
  {}
  ::hal::msg::HalBatteryMsg switch_state(::hal::msg::HalBatteryMsg::_switch_state_type arg)
  {
    msg_.switch_state = std::move(arg);
    return std::move(msg_);
  }

private:
  ::hal::msg::HalBatteryMsg msg_;
};

class Init_HalBatteryMsg_total_capacity
{
public:
  explicit Init_HalBatteryMsg_total_capacity(::hal::msg::HalBatteryMsg & msg)
  : msg_(msg)
  {}
  Init_HalBatteryMsg_switch_state total_capacity(::hal::msg::HalBatteryMsg::_total_capacity_type arg)
  {
    msg_.total_capacity = std::move(arg);
    return Init_HalBatteryMsg_switch_state(msg_);
  }

private:
  ::hal::msg::HalBatteryMsg msg_;
};

class Init_HalBatteryMsg_remain_capacity
{
public:
  explicit Init_HalBatteryMsg_remain_capacity(::hal::msg::HalBatteryMsg & msg)
  : msg_(msg)
  {}
  Init_HalBatteryMsg_total_capacity remain_capacity(::hal::msg::HalBatteryMsg::_remain_capacity_type arg)
  {
    msg_.remain_capacity = std::move(arg);
    return Init_HalBatteryMsg_total_capacity(msg_);
  }

private:
  ::hal::msg::HalBatteryMsg msg_;
};

class Init_HalBatteryMsg_cycle_count
{
public:
  explicit Init_HalBatteryMsg_cycle_count(::hal::msg::HalBatteryMsg & msg)
  : msg_(msg)
  {}
  Init_HalBatteryMsg_remain_capacity cycle_count(::hal::msg::HalBatteryMsg::_cycle_count_type arg)
  {
    msg_.cycle_count = std::move(arg);
    return Init_HalBatteryMsg_remain_capacity(msg_);
  }

private:
  ::hal::msg::HalBatteryMsg msg_;
};

class Init_HalBatteryMsg_battery_current
{
public:
  explicit Init_HalBatteryMsg_battery_current(::hal::msg::HalBatteryMsg & msg)
  : msg_(msg)
  {}
  Init_HalBatteryMsg_cycle_count battery_current(::hal::msg::HalBatteryMsg::_battery_current_type arg)
  {
    msg_.battery_current = std::move(arg);
    return Init_HalBatteryMsg_cycle_count(msg_);
  }

private:
  ::hal::msg::HalBatteryMsg msg_;
};

class Init_HalBatteryMsg_battery_status
{
public:
  Init_HalBatteryMsg_battery_status()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_HalBatteryMsg_battery_current battery_status(::hal::msg::HalBatteryMsg::_battery_status_type arg)
  {
    msg_.battery_status = std::move(arg);
    return Init_HalBatteryMsg_battery_current(msg_);
  }

private:
  ::hal::msg::HalBatteryMsg msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::hal::msg::HalBatteryMsg>()
{
  return hal::msg::builder::Init_HalBatteryMsg_battery_status();
}

}  // namespace hal

#endif  // HAL__MSG__DETAIL__HAL_BATTERY_MSG__BUILDER_HPP_
