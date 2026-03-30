// generated from rosidl_typesupport_fastrtps_cpp/resource/idl__type_support.cpp.em
// with input from hal:msg/HalAuxithrusterMsg.idl
// generated code does not contain a copyright notice
#include "hal/msg/detail/hal_auxithruster_msg__rosidl_typesupport_fastrtps_cpp.hpp"
#include "hal/msg/detail/hal_auxithruster_msg__struct.hpp"

#include <limits>
#include <stdexcept>
#include <string>
#include "rosidl_typesupport_cpp/message_type_support.hpp"
#include "rosidl_typesupport_fastrtps_cpp/identifier.hpp"
#include "rosidl_typesupport_fastrtps_cpp/message_type_support.h"
#include "rosidl_typesupport_fastrtps_cpp/message_type_support_decl.hpp"
#include "rosidl_typesupport_fastrtps_cpp/wstring_conversion.hpp"
#include "fastcdr/Cdr.h"


// forward declaration of message dependencies and their conversion functions

namespace hal
{

namespace msg
{

namespace typesupport_fastrtps_cpp
{

bool
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_hal
cdr_serialize(
  const hal::msg::HalAuxithrusterMsg & ros_message,
  eprosima::fastcdr::Cdr & cdr)
{
  // Member: rpm
  {
    cdr << ros_message.rpm;
  }
  // Member: current
  {
    cdr << ros_message.current;
  }
  // Member: voltage
  {
    cdr << ros_message.voltage;
  }
  // Member: temp
  {
    cdr << ros_message.temp;
  }
  // Member: esc_status
  {
    cdr << ros_message.esc_status;
  }
  // Member: fault_status
  {
    cdr << ros_message.fault_status;
  }
  return true;
}

bool
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_hal
cdr_deserialize(
  eprosima::fastcdr::Cdr & cdr,
  hal::msg::HalAuxithrusterMsg & ros_message)
{
  // Member: rpm
  {
    cdr >> ros_message.rpm;
  }

  // Member: current
  {
    cdr >> ros_message.current;
  }

  // Member: voltage
  {
    cdr >> ros_message.voltage;
  }

  // Member: temp
  {
    cdr >> ros_message.temp;
  }

  // Member: esc_status
  {
    cdr >> ros_message.esc_status;
  }

  // Member: fault_status
  {
    cdr >> ros_message.fault_status;
  }

  return true;
}  // NOLINT(readability/fn_size)

size_t
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_hal
get_serialized_size(
  const hal::msg::HalAuxithrusterMsg & ros_message,
  size_t current_alignment)
{
  size_t initial_alignment = current_alignment;

  const size_t padding = 4;
  const size_t wchar_size = 4;
  (void)padding;
  (void)wchar_size;

  // Member: rpm
  {
    size_t array_size = 6;
    size_t item_size = sizeof(ros_message.rpm[0]);
    current_alignment += array_size * item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }
  // Member: current
  {
    size_t array_size = 6;
    size_t item_size = sizeof(ros_message.current[0]);
    current_alignment += array_size * item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }
  // Member: voltage
  {
    size_t array_size = 6;
    size_t item_size = sizeof(ros_message.voltage[0]);
    current_alignment += array_size * item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }
  // Member: temp
  {
    size_t array_size = 6;
    size_t item_size = sizeof(ros_message.temp[0]);
    current_alignment += array_size * item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }
  // Member: esc_status
  {
    size_t array_size = 6;
    size_t item_size = sizeof(ros_message.esc_status[0]);
    current_alignment += array_size * item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }
  // Member: fault_status
  {
    size_t array_size = 6;
    size_t item_size = sizeof(ros_message.fault_status[0]);
    current_alignment += array_size * item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }

  return current_alignment - initial_alignment;
}

size_t
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_PUBLIC_hal
max_serialized_size_HalAuxithrusterMsg(
  bool & full_bounded,
  bool & is_plain,
  size_t current_alignment)
{
  size_t initial_alignment = current_alignment;

  const size_t padding = 4;
  const size_t wchar_size = 4;
  size_t last_member_size = 0;
  (void)last_member_size;
  (void)padding;
  (void)wchar_size;

  full_bounded = true;
  is_plain = true;


  // Member: rpm
  {
    size_t array_size = 6;

    last_member_size = array_size * sizeof(uint16_t);
    current_alignment += array_size * sizeof(uint16_t) +
      eprosima::fastcdr::Cdr::alignment(current_alignment, sizeof(uint16_t));
  }

  // Member: current
  {
    size_t array_size = 6;

    last_member_size = array_size * sizeof(uint16_t);
    current_alignment += array_size * sizeof(uint16_t) +
      eprosima::fastcdr::Cdr::alignment(current_alignment, sizeof(uint16_t));
  }

  // Member: voltage
  {
    size_t array_size = 6;

    last_member_size = array_size * sizeof(uint16_t);
    current_alignment += array_size * sizeof(uint16_t) +
      eprosima::fastcdr::Cdr::alignment(current_alignment, sizeof(uint16_t));
  }

  // Member: temp
  {
    size_t array_size = 6;

    last_member_size = array_size * sizeof(uint16_t);
    current_alignment += array_size * sizeof(uint16_t) +
      eprosima::fastcdr::Cdr::alignment(current_alignment, sizeof(uint16_t));
  }

  // Member: esc_status
  {
    size_t array_size = 6;

    last_member_size = array_size * sizeof(uint8_t);
    current_alignment += array_size * sizeof(uint8_t);
  }

  // Member: fault_status
  {
    size_t array_size = 6;

    last_member_size = array_size * sizeof(uint8_t);
    current_alignment += array_size * sizeof(uint8_t);
  }

  size_t ret_val = current_alignment - initial_alignment;
  if (is_plain) {
    // All members are plain, and type is not empty.
    // We still need to check that the in-memory alignment
    // is the same as the CDR mandated alignment.
    using DataType = hal::msg::HalAuxithrusterMsg;
    is_plain =
      (
      offsetof(DataType, fault_status) +
      last_member_size
      ) == ret_val;
  }

  return ret_val;
}

static bool _HalAuxithrusterMsg__cdr_serialize(
  const void * untyped_ros_message,
  eprosima::fastcdr::Cdr & cdr)
{
  auto typed_message =
    static_cast<const hal::msg::HalAuxithrusterMsg *>(
    untyped_ros_message);
  return cdr_serialize(*typed_message, cdr);
}

static bool _HalAuxithrusterMsg__cdr_deserialize(
  eprosima::fastcdr::Cdr & cdr,
  void * untyped_ros_message)
{
  auto typed_message =
    static_cast<hal::msg::HalAuxithrusterMsg *>(
    untyped_ros_message);
  return cdr_deserialize(cdr, *typed_message);
}

static uint32_t _HalAuxithrusterMsg__get_serialized_size(
  const void * untyped_ros_message)
{
  auto typed_message =
    static_cast<const hal::msg::HalAuxithrusterMsg *>(
    untyped_ros_message);
  return static_cast<uint32_t>(get_serialized_size(*typed_message, 0));
}

static size_t _HalAuxithrusterMsg__max_serialized_size(char & bounds_info)
{
  bool full_bounded;
  bool is_plain;
  size_t ret_val;

  ret_val = max_serialized_size_HalAuxithrusterMsg(full_bounded, is_plain, 0);

  bounds_info =
    is_plain ? ROSIDL_TYPESUPPORT_FASTRTPS_PLAIN_TYPE :
    full_bounded ? ROSIDL_TYPESUPPORT_FASTRTPS_BOUNDED_TYPE : ROSIDL_TYPESUPPORT_FASTRTPS_UNBOUNDED_TYPE;
  return ret_val;
}

static message_type_support_callbacks_t _HalAuxithrusterMsg__callbacks = {
  "hal::msg",
  "HalAuxithrusterMsg",
  _HalAuxithrusterMsg__cdr_serialize,
  _HalAuxithrusterMsg__cdr_deserialize,
  _HalAuxithrusterMsg__get_serialized_size,
  _HalAuxithrusterMsg__max_serialized_size
};

static rosidl_message_type_support_t _HalAuxithrusterMsg__handle = {
  rosidl_typesupport_fastrtps_cpp::typesupport_identifier,
  &_HalAuxithrusterMsg__callbacks,
  get_message_typesupport_handle_function,
};

}  // namespace typesupport_fastrtps_cpp

}  // namespace msg

}  // namespace hal

namespace rosidl_typesupport_fastrtps_cpp
{

template<>
ROSIDL_TYPESUPPORT_FASTRTPS_CPP_EXPORT_hal
const rosidl_message_type_support_t *
get_message_type_support_handle<hal::msg::HalAuxithrusterMsg>()
{
  return &hal::msg::typesupport_fastrtps_cpp::_HalAuxithrusterMsg__handle;
}

}  // namespace rosidl_typesupport_fastrtps_cpp

#ifdef __cplusplus
extern "C"
{
#endif

const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_cpp, hal, msg, HalAuxithrusterMsg)() {
  return &hal::msg::typesupport_fastrtps_cpp::_HalAuxithrusterMsg__handle;
}

#ifdef __cplusplus
}
#endif
