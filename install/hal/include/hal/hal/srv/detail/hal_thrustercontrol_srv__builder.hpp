// generated from rosidl_generator_cpp/resource/idl__builder.hpp.em
// with input from hal:srv/HalThrustercontrolSrv.idl
// generated code does not contain a copyright notice

#ifndef HAL__SRV__DETAIL__HAL_THRUSTERCONTROL_SRV__BUILDER_HPP_
#define HAL__SRV__DETAIL__HAL_THRUSTERCONTROL_SRV__BUILDER_HPP_

#include <algorithm>
#include <utility>

#include "hal/srv/detail/hal_thrustercontrol_srv__struct.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


namespace hal
{

namespace srv
{

namespace builder
{

class Init_HalThrustercontrolSrv_Request_command
{
public:
  Init_HalThrustercontrolSrv_Request_command()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  ::hal::srv::HalThrustercontrolSrv_Request command(::hal::srv::HalThrustercontrolSrv_Request::_command_type arg)
  {
    msg_.command = std::move(arg);
    return std::move(msg_);
  }

private:
  ::hal::srv::HalThrustercontrolSrv_Request msg_;
};

}  // namespace builder

}  // namespace srv

template<typename MessageType>
auto build();

template<>
inline
auto build<::hal::srv::HalThrustercontrolSrv_Request>()
{
  return hal::srv::builder::Init_HalThrustercontrolSrv_Request_command();
}

}  // namespace hal


namespace hal
{

namespace srv
{

namespace builder
{

class Init_HalThrustercontrolSrv_Response_message
{
public:
  explicit Init_HalThrustercontrolSrv_Response_message(::hal::srv::HalThrustercontrolSrv_Response & msg)
  : msg_(msg)
  {}
  ::hal::srv::HalThrustercontrolSrv_Response message(::hal::srv::HalThrustercontrolSrv_Response::_message_type arg)
  {
    msg_.message = std::move(arg);
    return std::move(msg_);
  }

private:
  ::hal::srv::HalThrustercontrolSrv_Response msg_;
};

class Init_HalThrustercontrolSrv_Response_success
{
public:
  Init_HalThrustercontrolSrv_Response_success()
  : msg_(::rosidl_runtime_cpp::MessageInitialization::SKIP)
  {}
  Init_HalThrustercontrolSrv_Response_message success(::hal::srv::HalThrustercontrolSrv_Response::_success_type arg)
  {
    msg_.success = std::move(arg);
    return Init_HalThrustercontrolSrv_Response_message(msg_);
  }

private:
  ::hal::srv::HalThrustercontrolSrv_Response msg_;
};

}  // namespace builder

}  // namespace srv

template<typename MessageType>
auto build();

template<>
inline
auto build<::hal::srv::HalThrustercontrolSrv_Response>()
{
  return hal::srv::builder::Init_HalThrustercontrolSrv_Response_success();
}

}  // namespace hal

#endif  // HAL__SRV__DETAIL__HAL_THRUSTERCONTROL_SRV__BUILDER_HPP_
