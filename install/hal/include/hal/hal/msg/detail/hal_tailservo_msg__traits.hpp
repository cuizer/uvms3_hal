// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from hal:msg/HalTailservoMsg.idl
// generated code does not contain a copyright notice

#ifndef HAL__MSG__DETAIL__HAL_TAILSERVO_MSG__TRAITS_HPP_
#define HAL__MSG__DETAIL__HAL_TAILSERVO_MSG__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "hal/msg/detail/hal_tailservo_msg__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace hal
{

namespace msg
{

inline void to_flow_style_yaml(
  const HalTailservoMsg & msg,
  std::ostream & out)
{
  out << "{";
  // member: voltage
  {
    if (msg.voltage.size() == 0) {
      out << "voltage: []";
    } else {
      out << "voltage: [";
      size_t pending_items = msg.voltage.size();
      for (auto item : msg.voltage) {
        rosidl_generator_traits::value_to_yaml(item, out);
        if (--pending_items > 0) {
          out << ", ";
        }
      }
      out << "]";
    }
    out << ", ";
  }

  // member: current
  {
    if (msg.current.size() == 0) {
      out << "current: []";
    } else {
      out << "current: [";
      size_t pending_items = msg.current.size();
      for (auto item : msg.current) {
        rosidl_generator_traits::value_to_yaml(item, out);
        if (--pending_items > 0) {
          out << ", ";
        }
      }
      out << "]";
    }
    out << ", ";
  }

  // member: power
  {
    if (msg.power.size() == 0) {
      out << "power: []";
    } else {
      out << "power: [";
      size_t pending_items = msg.power.size();
      for (auto item : msg.power) {
        rosidl_generator_traits::value_to_yaml(item, out);
        if (--pending_items > 0) {
          out << ", ";
        }
      }
      out << "]";
    }
    out << ", ";
  }

  // member: temperature
  {
    if (msg.temperature.size() == 0) {
      out << "temperature: []";
    } else {
      out << "temperature: [";
      size_t pending_items = msg.temperature.size();
      for (auto item : msg.temperature) {
        rosidl_generator_traits::value_to_yaml(item, out);
        if (--pending_items > 0) {
          out << ", ";
        }
      }
      out << "]";
    }
    out << ", ";
  }

  // member: status
  {
    if (msg.status.size() == 0) {
      out << "status: []";
    } else {
      out << "status: [";
      size_t pending_items = msg.status.size();
      for (auto item : msg.status) {
        rosidl_generator_traits::value_to_yaml(item, out);
        if (--pending_items > 0) {
          out << ", ";
        }
      }
      out << "]";
    }
  }
  out << "}";
}  // NOLINT(readability/fn_size)

inline void to_block_style_yaml(
  const HalTailservoMsg & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: voltage
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.voltage.size() == 0) {
      out << "voltage: []\n";
    } else {
      out << "voltage:\n";
      for (auto item : msg.voltage) {
        if (indentation > 0) {
          out << std::string(indentation, ' ');
        }
        out << "- ";
        rosidl_generator_traits::value_to_yaml(item, out);
        out << "\n";
      }
    }
  }

  // member: current
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.current.size() == 0) {
      out << "current: []\n";
    } else {
      out << "current:\n";
      for (auto item : msg.current) {
        if (indentation > 0) {
          out << std::string(indentation, ' ');
        }
        out << "- ";
        rosidl_generator_traits::value_to_yaml(item, out);
        out << "\n";
      }
    }
  }

  // member: power
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.power.size() == 0) {
      out << "power: []\n";
    } else {
      out << "power:\n";
      for (auto item : msg.power) {
        if (indentation > 0) {
          out << std::string(indentation, ' ');
        }
        out << "- ";
        rosidl_generator_traits::value_to_yaml(item, out);
        out << "\n";
      }
    }
  }

  // member: temperature
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.temperature.size() == 0) {
      out << "temperature: []\n";
    } else {
      out << "temperature:\n";
      for (auto item : msg.temperature) {
        if (indentation > 0) {
          out << std::string(indentation, ' ');
        }
        out << "- ";
        rosidl_generator_traits::value_to_yaml(item, out);
        out << "\n";
      }
    }
  }

  // member: status
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.status.size() == 0) {
      out << "status: []\n";
    } else {
      out << "status:\n";
      for (auto item : msg.status) {
        if (indentation > 0) {
          out << std::string(indentation, ' ');
        }
        out << "- ";
        rosidl_generator_traits::value_to_yaml(item, out);
        out << "\n";
      }
    }
  }
}  // NOLINT(readability/fn_size)

inline std::string to_yaml(const HalTailservoMsg & msg, bool use_flow_style = false)
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
  const hal::msg::HalTailservoMsg & msg,
  std::ostream & out, size_t indentation = 0)
{
  hal::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use hal::msg::to_yaml() instead")]]
inline std::string to_yaml(const hal::msg::HalTailservoMsg & msg)
{
  return hal::msg::to_yaml(msg);
}

template<>
inline const char * data_type<hal::msg::HalTailservoMsg>()
{
  return "hal::msg::HalTailservoMsg";
}

template<>
inline const char * name<hal::msg::HalTailservoMsg>()
{
  return "hal/msg/HalTailservoMsg";
}

template<>
struct has_fixed_size<hal::msg::HalTailservoMsg>
  : std::integral_constant<bool, true> {};

template<>
struct has_bounded_size<hal::msg::HalTailservoMsg>
  : std::integral_constant<bool, true> {};

template<>
struct is_message<hal::msg::HalTailservoMsg>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // HAL__MSG__DETAIL__HAL_TAILSERVO_MSG__TRAITS_HPP_
