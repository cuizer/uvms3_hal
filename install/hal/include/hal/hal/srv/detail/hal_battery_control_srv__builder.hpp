// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from hal:srv/HalBatteryControlSrv.idl
// generated code does not contain a copyright notice

#ifndef HAL__SRV__DETAIL__HAL_BATTERY_CONTROL_SRV__BUILDER_HPP_
#define HAL__SRV__DETAIL__HAL_BATTERY_CONTROL_SRV__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "hal/srv/detail/hal_battery_control_srv__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace hal
{

namespace srv
{

namespace builder
{

class Init_HalBatteryControlSrv_Request_command
{
public:
  Init_HalBatteryControlSrv_Request_command()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  ::hal::srv::HalBatteryControlSrv_Request command(::hal::srv::HalBatteryControlSrv_Request::_command_type arg)
  {
    msg_.command = std::move(arg);
    return std::move(msg_);
  }

private:
  ::hal::srv::HalBatteryControlSrv_Request msg_;
};

}  // namespace builder

}  // namespace srv

template<typename MessageType>
auto build();

template<>
inline
auto build<::hal::srv::HalBatteryControlSrv_Request>()
{
  return hal::srv::builder::Init_HalBatteryControlSrv_Request_command();
}

}  // namespace hal


namespace hal
{

namespace srv
{

namespace builder
{

class Init_HalBatteryControlSrv_Response_message
{
public:
  explicit Init_HalBatteryControlSrv_Response_message(::hal::srv::HalBatteryControlSrv_Response & msg)
  : msg_(msg)
  {}
  ::hal::srv::HalBatteryControlSrv_Response message(::hal::srv::HalBatteryControlSrv_Response::_message_type arg)
  {
    msg_.message = std::move(arg);
    return std::move(msg_);
  }

private:
  ::hal::srv::HalBatteryControlSrv_Response msg_;
};

class Init_HalBatteryControlSrv_Response_success
{
public:
  Init_HalBatteryControlSrv_Response_success()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_HalBatteryControlSrv_Response_message success(::hal::srv::HalBatteryControlSrv_Response::_success_type arg)
  {
    msg_.success = std::move(arg);
    return Init_HalBatteryControlSrv_Response_message(msg_);
  }

private:
  ::hal::srv::HalBatteryControlSrv_Response msg_;
};

}  // namespace builder

}  // namespace srv

template<typename MessageType>
auto build();

template<>
inline
auto build<::hal::srv::HalBatteryControlSrv_Response>()
{
  return hal::srv::builder::Init_HalBatteryControlSrv_Response_success();
}

}  // namespace hal

#endif  // HAL__SRV__DETAIL__HAL_BATTERY_CONTROL_SRV__BUILDER_HPP_
