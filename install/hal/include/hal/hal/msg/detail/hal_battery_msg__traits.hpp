// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from hal:msg/HalBatteryMsg.idl
// generated code does not contain a copyright notice

#ifndef HAL__MSG__DETAIL__HAL_BATTERY_MSG__TRAITS_HPP_
#define HAL__MSG__DETAIL__HAL_BATTERY_MSG__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "hal/msg/detail/hal_battery_msg__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace hal
{

namespace msg
{

inline void to_flow_style_yaml(
  const HalBatteryMsg & msg,
  std::ostream & out)
{
  out << "{";
  // member: battery_status
  {
    out << "battery_status: ";
    rosidl_generator_traits::character_value_to_yaml(msg.battery_status, out);
    out << ", ";
  }

  // member: battery_current
  {
    out << "battery_current: ";
    rosidl_generator_traits::value_to_yaml(msg.battery_current, out);
    out << ", ";
  }

  // member: cycle_count
  {
    out << "cycle_count: ";
    rosidl_generator_traits::value_to_yaml(msg.cycle_count, out);
    out << ", ";
  }

  // member: remain_capacity
  {
    out << "remain_capacity: ";
    rosidl_generator_traits::value_to_yaml(msg.remain_capacity, out);
    out << ", ";
  }

  // member: total_capacity
  {
    out << "total_capacity: ";
    rosidl_generator_traits::value_to_yaml(msg.total_capacity, out);
    out << ", ";
  }

  // member: switch_state
  {
    out << "switch_state: ";
    rosidl_generator_traits::character_value_to_yaml(msg.switch_state, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const HalBatteryMsg & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: battery_status
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "battery_status: ";
    rosidl_generator_traits::character_value_to_yaml(msg.battery_status, out);
    out << "\n";
  }

  // member: battery_current
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "battery_current: ";
    rosidl_generator_traits::value_to_yaml(msg.battery_current, out);
    out << "\n";
  }

  // member: cycle_count
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "cycle_count: ";
    rosidl_generator_traits::value_to_yaml(msg.cycle_count, out);
    out << "\n";
  }

  // member: remain_capacity
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "remain_capacity: ";
    rosidl_generator_traits::value_to_yaml(msg.remain_capacity, out);
    out << "\n";
  }

  // member: total_capacity
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "total_capacity: ";
    rosidl_generator_traits::value_to_yaml(msg.total_capacity, out);
    out << "\n";
  }

  // member: switch_state
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "switch_state: ";
    rosidl_generator_traits::character_value_to_yaml(msg.switch_state, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const HalBatteryMsg & msg, bool use_flow_style = false)
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
  const hal::msg::HalBatteryMsg & msg,
  std::ostream & out, size_t indentation = 0)
{
  hal::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use hal::msg::to_yaml() instead")]]
inline std::string to_yaml(const hal::msg::HalBatteryMsg & msg)
{
  return hal::msg::to_yaml(msg);
}

template<>
inline const char * data_type<hal::msg::HalBatteryMsg>()
{
  return "hal::msg::HalBatteryMsg";
}

template<>
inline const char * name<hal::msg::HalBatteryMsg>()
{
  return "hal/msg/HalBatteryMsg";
}

template<>
struct has_fixed_size<hal::msg::HalBatteryMsg>
  : std::integral_constant<bool, true> {};

template<>
struct has_bounded_size<hal::msg::HalBatteryMsg>
  : std::integral_constant<bool, true> {};

template<>
struct is_message<hal::msg::HalBatteryMsg>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // HAL__MSG__DETAIL__HAL_BATTERY_MSG__TRAITS_HPP_
