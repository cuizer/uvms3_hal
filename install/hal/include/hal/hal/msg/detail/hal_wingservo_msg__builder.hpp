// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from hal:msg/HalWingservoMsg.idl
// generated code does not contain a copyright notice

#ifndef HAL__MSG__DETAIL__HAL_WINGSERVO_MSG__BUILDER_HPP_
#define HAL__MSG__DETAIL__HAL_WINGSERVO_MSG__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "hal/msg/detail/hal_wingservo_msg__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace hal
{

namespace msg
{

namespace builder
{

class Init_HalWingservoMsg_status
{
public:
  explicit Init_HalWingservoMsg_status(::hal::msg::HalWingservoMsg & msg)
  : msg_(msg)
  {}
  ::hal::msg::HalWingservoMsg status(::hal::msg::HalWingservoMsg::_status_type arg)
  {
    msg_.status = std::move(arg);
    return std::move(msg_);
  }

private:
  ::hal::msg::HalWingservoMsg msg_;
};

class Init_HalWingservoMsg_temperature
{
public:
  explicit Init_HalWingservoMsg_temperature(::hal::msg::HalWingservoMsg & msg)
  : msg_(msg)
  {}
  Init_HalWingservoMsg_status temperature(::hal::msg::HalWingservoMsg::_temperature_type arg)
  {
    msg_.temperature = std::move(arg);
    return Init_HalWingservoMsg_status(msg_);
  }

private:
  ::hal::msg::HalWingservoMsg msg_;
};

class Init_HalWingservoMsg_power
{
public:
  explicit Init_HalWingservoMsg_power(::hal::msg::HalWingservoMsg & msg)
  : msg_(msg)
  {}
  Init_HalWingservoMsg_temperature power(::hal::msg::HalWingservoMsg::_power_type arg)
  {
    msg_.power = std::move(arg);
    return Init_HalWingservoMsg_temperature(msg_);
  }

private:
  ::hal::msg::HalWingservoMsg msg_;
};

class Init_HalWingservoMsg_current
{
public:
  explicit Init_HalWingservoMsg_current(::hal::msg::HalWingservoMsg & msg)
  : msg_(msg)
  {}
  Init_HalWingservoMsg_power current(::hal::msg::HalWingservoMsg::_current_type arg)
  {
    msg_.current = std::move(arg);
    return Init_HalWingservoMsg_power(msg_);
  }

private:
  ::hal::msg::HalWingservoMsg msg_;
};

class Init_HalWingservoMsg_voltage
{
public:
  Init_HalWingservoMsg_voltage()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_HalWingservoMsg_current voltage(::hal::msg::HalWingservoMsg::_voltage_type arg)
  {
    msg_.voltage = std::move(arg);
    return Init_HalWingservoMsg_current(msg_);
  }

private:
  ::hal::msg::HalWingservoMsg msg_;
};

}  // namespace builder

}  // namespace msg

template<typename MessageType>
auto build();

template<>
inline
auto build<::hal::msg::HalWingservoMsg>()
{
  return hal::msg::builder::Init_HalWingservoMsg_voltage();
}

}  // namespace hal

#endif  // HAL__MSG__DETAIL__HAL_WINGSERVO_MSG__BUILDER_HPP_
