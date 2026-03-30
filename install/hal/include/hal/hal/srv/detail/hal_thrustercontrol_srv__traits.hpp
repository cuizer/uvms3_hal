// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from hal:srv/HalThrustercontrolSrv.idl
// generated code does not contain a copyright notice

#ifndef HAL__SRV__DETAIL__HAL_THRUSTERCONTROL_SRV__TRAITS_HPP_
#define HAL__SRV__DETAIL__HAL_THRUSTERCONTROL_SRV__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "hal/srv/detail/hal_thrustercontrol_srv__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace hal
{

namespace srv
{

inline void to_flow_style_yaml(
  const HalThrustercontrolSrv_Request & msg,
  std::ostream & out)
{
  out << "{";
  // member: command
  {
    out << "command: ";
    rosidl_generator_traits::value_to_yaml(msg.command, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const HalThrustercontrolSrv_Request & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: command
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "command: ";
    rosidl_generator_traits::value_to_yaml(msg.command, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const HalThrustercontrolSrv_Request & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace srv

}  // namespace hal

namespace rosidl_generator_traits
{

[[deprecated("use hal::srv::to_block_style_yaml() instead")]]
inline void to_yaml(
  const hal::srv::HalThrustercontrolSrv_Request & msg,
  std::ostream & out, size_t indentation = 0)
{
  hal::srv::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use hal::srv::to_yaml() instead")]]
inline std::string to_yaml(const hal::srv::HalThrustercontrolSrv_Request & msg)
{
  return hal::srv::to_yaml(msg);
}

template<>
inline const char * data_type<hal::srv::HalThrustercontrolSrv_Request>()
{
  return "hal::srv::HalThrustercontrolSrv_Request";
}

template<>
inline const char * name<hal::srv::HalThrustercontrolSrv_Request>()
{
  return "hal/srv/HalThrustercontrolSrv_Request";
}

template<>
struct has_fixed_size<hal::srv::HalThrustercontrolSrv_Request>
  : std::integral_constant<bool, true> {};

template<>
struct has_bounded_size<hal::srv::HalThrustercontrolSrv_Request>
  : std::integral_constant<bool, true> {};

template<>
struct is_message<hal::srv::HalThrustercontrolSrv_Request>
  : std::true_type {};

}  // namespace rosidl_generator_traits

namespace hal
{

namespace srv
{

inline void to_flow_style_yaml(
  const HalThrustercontrolSrv_Response & msg,
  std::ostream & out)
{
  out << "{";
  // member: success
  {
    out << "success: ";
    rosidl_generator_traits::value_to_yaml(msg.success, out);
    out << ", ";
  }

  // member: message
  {
    out << "message: ";
    rosidl_generator_traits::value_to_yaml(msg.message, out);
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const HalThrustercontrolSrv_Response & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: success
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "success: ";
    rosidl_generator_traits::value_to_yaml(msg.success, out);
    out << "\n";
  }

  // member: message
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    out << "message: ";
    rosidl_generator_traits::value_to_yaml(msg.message, out);
    out << "\n";
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const HalThrustercontrolSrv_Response & msg, bool use_flow_style = false)
{
  std::ostringstream out;
  if (use_flow_style) {
    to_flow_style_yaml(msg, out);
  } else {
    to_block_style_yaml(msg, out);
  }
  return out.str();
}

}  // namespace srv

}  // namespace hal

namespace rosidl_generator_traits
{

[[deprecated("use hal::srv::to_block_style_yaml() instead")]]
inline void to_yaml(
  const hal::srv::HalThrustercontrolSrv_Response & msg,
  std::ostream & out, size_t indentation = 0)
{
  hal::srv::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use hal::srv::to_yaml() instead")]]
inline std::string to_yaml(const hal::srv::HalThrustercontrolSrv_Response & msg)
{
  return hal::srv::to_yaml(msg);
}

template<>
inline const char * data_type<hal::srv::HalThrustercontrolSrv_Response>()
{
  return "hal::srv::HalThrustercontrolSrv_Response";
}

template<>
inline const char * name<hal::srv::HalThrustercontrolSrv_Response>()
{
  return "hal/srv/HalThrustercontrolSrv_Response";
}

template<>
struct has_fixed_size<hal::srv::HalThrustercontrolSrv_Response>
  : std::integral_constant<bool, false> {};

template<>
struct has_bounded_size<hal::srv::HalThrustercontrolSrv_Response>
  : std::integral_constant<bool, false> {};

template<>
struct is_message<hal::srv::HalThrustercontrolSrv_Response>
  : std::true_type {};

}  // namespace rosidl_generator_traits

namespace rosidl_generator_traits
{

template<>
inline const char * data_type<hal::srv::HalThrustercontrolSrv>()
{
  return "hal::srv::HalThrustercontrolSrv";
}

template<>
inline const char * name<hal::srv::HalThrustercontrolSrv>()
{
  return "hal/srv/HalThrustercontrolSrv";
}

template<>
struct has_fixed_size<hal::srv::HalThrustercontrolSrv>
  : std::integral_constant<
    bool,
    has_fixed_size<hal::srv::HalThrustercontrolSrv_Request>::value &&
    has_fixed_size<hal::srv::HalThrustercontrolSrv_Response>::value
  >
{
};

template<>
struct has_bounded_size<hal::srv::HalThrustercontrolSrv>
  : std::integral_constant<
    bool,
    has_bounded_size<hal::srv::HalThrustercontrolSrv_Request>::value &&
    has_bounded_size<hal::srv::HalThrustercontrolSrv_Response>::value
  >
{
};

template<>
struct is_service<hal::srv::HalThrustercontrolSrv>
  : std::true_type
{
};

template<>
struct is_service_request<hal::srv::HalThrustercontrolSrv_Request>
  : std::true_type
{
};

template<>
struct is_service_response<hal::srv::HalThrustercontrolSrv_Response>
  : std::true_type
{
};

}  // namespace rosidl_generator_traits

#endif  // HAL__SRV__DETAIL__HAL_THRUSTERCONTROL_SRV__TRAITS_HPP_
