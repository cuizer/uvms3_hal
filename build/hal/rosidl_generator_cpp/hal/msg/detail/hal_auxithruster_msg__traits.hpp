// generated from rosidl_generator_cpp/resource/idl__traits.hpp.em
// with input from hal:msg/HalAuxithrusterMsg.idl
// generated code does not contain a copyright notice

#ifndef HAL__MSG__DETAIL__HAL_AUXITHRUSTER_MSG__TRAITS_HPP_
#define HAL__MSG__DETAIL__HAL_AUXITHRUSTER_MSG__TRAITS_HPP_

#include <stdint.h>

#include <sstream>
#include <string>
#include <type_traits>

#include "hal/msg/detail/hal_auxithruster_msg__struct.hpp"
#include "rosidl_runtime_cpp/traits.hpp"

namespace hal
{

namespace msg
{

inline void to_flow_style_yaml(
  const HalAuxithrusterMsg & msg,
  std::ostream & out)
{
  out << "{";
  // member: rpm
  {
    if (msg.rpm.size() == 0) {
      out << "rpm: []";
    } else {
      out << "rpm: [";
      size_t pending_items = msg.rpm.size();
      for (auto item : msg.rpm) {
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

  // member: temp
  {
    if (msg.temp.size() == 0) {
      out << "temp: []";
    } else {
      out << "temp: [";
      size_t pending_items = msg.temp.size();
      for (auto item : msg.temp) {
        rosidl_generator_traits::value_to_yaml(item, out);
        if (--pending_items > 0) {
          out << ", ";
        }
      }
      out << "]";
    }
    out << ", ";
  }

  // member: esc_status
  {
    if (msg.esc_status.size() == 0) {
      out << "esc_status: []";
    } else {
      out << "esc_status: [";
      size_t pending_items = msg.esc_status.size();
      for (auto item : msg.esc_status) {
        rosidl_generator_traits::value_to_yaml(item, out);
        if (--pending_items > 0) {
          out << ", ";
        }
      }
      out << "]";
    }
    out << ", ";
  }

  // member: fault_status
  {
    if (msg.fault_status.size() == 0) {
      out << "fault_status: []";
    } else {
      out << "fault_status: [";
      size_t pending_items = msg.fault_status.size();
      for (auto item : msg.fault_status) {
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
  const HalAuxithrusterMsg & msg,
  std::ostream & out, size_t indentation = 0)
{
  // member: rpm
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.rpm.size() == 0) {
      out << "rpm: []\n";
    } else {
      out << "rpm:\n";
      for (auto item : msg.rpm) {
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

  // member: temp
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.temp.size() == 0) {
      out << "temp: []\n";
    } else {
      out << "temp:\n";
      for (auto item : msg.temp) {
        if (indentation > 0) {
          out << std::string(indentation, ' ');
        }
        out << "- ";
        rosidl_generator_traits::value_to_yaml(item, out);
        out << "\n";
      }
    }
  }

  // member: esc_status
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.esc_status.size() == 0) {
      out << "esc_status: []\n";
    } else {
      out << "esc_status:\n";
      for (auto item : msg.esc_status) {
        if (indentation > 0) {
          out << std::string(indentation, ' ');
        }
        out << "- ";
        rosidl_generator_traits::value_to_yaml(item, out);
        out << "\n";
      }
    }
  }

  // member: fault_status
  {
    if (indentation > 0) {
      out << std::string(indentation, ' ');
    }
    if (msg.fault_status.size() == 0) {
      out << "fault_status: []\n";
    } else {
      out << "fault_status:\n";
      for (auto item : msg.fault_status) {
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

inline std::string to_yaml(const HalAuxithrusterMsg & msg, bool use_flow_style = false)
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
  const hal::msg::HalAuxithrusterMsg & msg,
  std::ostream & out, size_t indentation = 0)
{
  hal::msg::to_block_style_yaml(msg, out, indentation);
}

[[deprecated("use hal::msg::to_yaml() instead")]]
inline std::string to_yaml(const hal::msg::HalAuxithrusterMsg & msg)
{
  return hal::msg::to_yaml(msg);
}

template<>
inline const char * data_type<hal::msg::HalAuxithrusterMsg>()
{
  return "hal::msg::HalAuxithrusterMsg";
}

template<>
inline const char * name<hal::msg::HalAuxithrusterMsg>()
{
  return "hal/msg/HalAuxithrusterMsg";
}

template<>
struct has_fixed_size<hal::msg::HalAuxithrusterMsg>
  : std::integral_constant<bool, true> {};

template<>
struct has_bounded_size<hal::msg::HalAuxithrusterMsg>
  : std::integral_constant<bool, true> {};

template<>
struct is_message<hal::msg::HalAuxithrusterMsg>
  : std::true_type {};

}  // namespace rosidl_generator_traits

#endif  // HAL__MSG__DETAIL__HAL_AUXITHRUSTER_MSG__TRAITS_HPP_
