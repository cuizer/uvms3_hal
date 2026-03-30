// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from hal:msg/HalMainthrusterMsg.idl
// generated code does not contain a copyright notice

#ifndef HAL__MSG__DETAIL__HAL_MAINTHRUSTER_MSG__TRAITS_HPP_
#define HAL__MSG__DETAIL__HAL_MAINTHRUSTER_MSG__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "hal/msg/detail/hal_mainthruster_msg__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace hal
{

namespace msg
{

inline void to_flow_style_yaml(
  const HalMainthrusterMsg & msg,
  std::ostream & out)
{
  out << "{";
  // member: rpm
  {
    out << "rpm: ";
    rosidl_generator_traits::value_to_yaml(msg.rpm, out);
    out << ", ";
  }

  // member: current
  {
    out << "current: ";
    rosidl_generator_traits::value_to_yaml(msg.current, out);
    out << ", ";
  }

  // member: voltage
  {
    out << "voltage: ";
    rosidl_generator_traits::value_to_yaml(msg.voltage, out);
    out << ", ";
  }

  // member: fault_status
  {
    out << "fault_status: ";
    rosidl_generator_traits::value_to_yaml(msg.fault_status, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const HalMainthrusterMsg & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: rpm
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "rpm: ";
    rosidl_generator_traits::value_to_yaml(msg.rpm, out);
    out << "\n";
  }

  // member: current
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "current: ";
    rosidl_generator_traits::value_to_yaml(msg.current, out);
    out << "\n";
  }

  // member: voltage
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "voltage: ";
    rosidl_generator_traits::value_to_yaml(msg.voltage, out);
    out << "\n";
  }

  // member: fault_status
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "fault_status: ";
    rosidl_generator_traits::value_to_yaml(msg.fault_status, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const HalMainthrusterMsg & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace msg

}  // namespace hal

namespace rosidl_generator_traits
{

[[deprecated("use hal::msg::to_block_style_yaml() instead")]]
inline void to_yaml(
  const hal::msg::HalMainthrusterMsg & msg,
  std::ostream & out, size_t indentation = 0)
{
  hal::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use hal::msg::to_yaml() instead")]]
inline std::string to_yaml(const hal::msg::HalMainthrusterMsg & msg)
{
  return hal::msg::to_yaml(msg);
}

template<>
inline const char * data_type<hal::msg::HalMainthrusterMsg>()
{
  return "hal::msg::HalMainthrusterMsg";
}

template<>
inline const char * name<hal::msg::HalMainthrusterMsg>()
{
  return "hal/msg/HalMainthrusterMsg";
}

template<>
struct has_fixed_size<hal::msg::HalMainthrusterMsg>
  : std::integral_constant<bool, true> {};

template<>
struct has_bounded_size<hal::msg::HalMainthrusterMsg>
  : std::integral_constant<bool, true> {};

template<>
struct is_message<hal::msg::HalMainthrusterMsg>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // HAL__MSG__DETAIL__HAL_MAINTHRUSTER_MSG__TRAITS_HPP_
