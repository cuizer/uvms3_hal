// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from hal:msg/HalTailservoMsg.idl
// generated code does not contain a copyright notice

#ifndef HAL__MSG__DETAIL__HAL_TAILSERVO_MSG__BUILDER_HPP_
#define HAL__MSG__DETAIL__HAL_TAILSERVO_MSG__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "hal/msg/detail/hal_tailservo_msg__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace hal
{

namespace msg
{

namespace builder
{

class Init_HalTailservoMsg_status
{
public:
  explicit Init_HalTailservoMsg_status(::hal::msg::HalTailservoMsg & msg)
  : msg_(msg)
  {}
  ::hal::msg::HalTailservoMsg status(::hal::msg::HalTailservoMsg::_status_type arg)
  {
    msg_.status = std::move(arg);
    return std::move(msg_);
  }

private:
  ::hal::msg::HalTailservoMsg msg_;
};

class Init_HalTailservoMsg_temperature
{
public:
  explicit Init_HalTailservoMsg_temperature(::hal::msg::HalTailservoMsg & msg)
  : msg_(msg)
  {}
  Init_HalTailservoMsg_status temperature(::hal::msg::HalTailservoMsg::_temperature_type arg)
  {
    msg_.temperature = std::move(arg);
    return Init_HalTailservoMsg_status(msg_);
  }

private:
  ::hal::msg::HalTailservoMsg msg_;
};

class Init_HalTailservoMsg_power
{
public:
  explicit Init_HalTailservoMsg_power(::hal::msg::HalTailservoMsg & msg)
  : msg_(msg)
  {}
  Init_HalTailservoMsg_temperature power(::hal::msg::HalTailservoMsg::_power_type arg)
  {
    msg_.power = std::move(arg);
    return Init_HalTailservoMsg_temperature(msg_);
  }

private:
  ::hal::msg::HalTailservoMsg msg_;
};

class Init_HalTailservoMsg_current
{
public:
  explicit Init_HalTailservoMsg_current(::hal::msg::HalTailservoMsg & msg)
  : msg_(msg)
  {}
  Init_HalTailservoMsg_power current(::hal::msg::HalTailservoMsg::_current_type arg)
  {
    msg_.current = std::move(arg);
    return Init_HalTailservoMsg_power(msg_);
  }

private:
  ::hal::msg::HalTailservoMsg msg_;
};

class Init_HalTailservoMsg_voltage
{
public:
  Init_HalTailservoMsg_voltage()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_HalTailservoMsg_current voltage(::hal::msg::HalTailservoMsg::_voltage_type arg)
  {
    msg_.voltage = std::move(arg);
    return Init_HalTailservoMsg_current(msg_);
  }

private:
  ::hal::msg::HalTailservoMsg msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::hal::msg::HalTailservoMsg>()
{
  return hal::msg::builder::Init_HalTailservoMsg_voltage();
}

}  // namespace hal

#endif  // HAL__MSG__DETAIL__HAL_TAILSERVO_MSG__BUILDER_HPP_
