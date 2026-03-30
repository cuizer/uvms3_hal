// generated from rosidl_typesupport_fastrtps_c/resource/idl__type_support_c.cpp.em
// with input from hal:msg/HalBatteryMsg.idl
// generated code does not contain a copyright notice
#include "hal/msg/detail/hal_battery_msg__rosidl_typesupport_fastrtps_c.h"


#include <cassert>
#include <limits>
#include <string>
#include "rosidl_typesupport_fastrtps_c/identifier.h"
#include "rosidl_typesupport_fastrtps_c/wstring_conversion.hpp"
#include "rosidl_typesupport_fastrtps_cpp/message_type_support.h"
#include "hal/msg/rosidl_typesupport_fastrtps_c__visibility_control.h"
#include "hal/msg/detail/hal_battery_msg__struct.h"
#include "hal/msg/detail/hal_battery_msg__functions.h"
#include "fastcdr/Cdr.h"

#ifndef _WIN32
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-parameter"
# ifdef __clang__
#  pragma clang diagnostic ignored "-Wdeprecated-register"
#  pragma clang diagnostic ignored "-Wreturn-type-c-linkage"
# endif
#endif
#ifndef _WIN32
# pragma GCC diagnostic pop
#endif

// includes and forward declarations of message dependencies and their conversion functions

#if defined(__cplusplus)
extern "C"
{
#endif


// forward declare type support functions


using _HalBatteryMsg__ros_msg_type = hal__msg__HalBatteryMsg;

static bool _HalBatteryMsg__cdr_serialize(
  const void * untyped_ros_message,
  eprosima::fastcdr::Cdr & cdr)
{
  if (!untyped_ros_message) {
    fprintf(stderr, "ros message handle is null\n");
    return false;
  }
  const _HalBatteryMsg__ros_msg_type * ros_message = static_cast<const _HalBatteryMsg__ros_msg_type *>(untyped_ros_message);
  // Field name: battery_status
  {
    cdr << ros_message->battery_status;
  }

  // Field name: battery_current
  {
    cdr << ros_message->battery_current;
  }

  // Field name: cycle_count
  {
    cdr << ros_message->cycle_count;
  }

  // Field name: remain_capacity
  {
    cdr << ros_message->remain_capacity;
  }

  // Field name: total_capacity
  {
    cdr << ros_message->total_capacity;
  }

  // Field name: switch_state
  {
    cdr << ros_message->switch_state;
  }

  return true;
}

static bool _HalBatteryMsg__cdr_deserialize(
  eprosima::fastcdr::Cdr & cdr,
  void * untyped_ros_message)
{
  if (!untyped_ros_message) {
    fprintf(stderr, "ros message handle is null\n");
    return false;
  }
  _HalBatteryMsg__ros_msg_type * ros_message = static_cast<_HalBatteryMsg__ros_msg_type *>(untyped_ros_message);
  // Field name: battery_status
  {
    cdr >> ros_message->battery_status;
  }

  // Field name: battery_current
  {
    cdr >> ros_message->battery_current;
  }

  // Field name: cycle_count
  {
    cdr >> ros_message->cycle_count;
  }

  // Field name: remain_capacity
  {
    cdr >> ros_message->remain_capacity;
  }

  // Field name: total_capacity
  {
    cdr >> ros_message->total_capacity;
  }

  // Field name: switch_state
  {
    cdr >> ros_message->switch_state;
  }

  return true;
}  // NOLINT(readability/fn_size)

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_hal
size_t get_serialized_size_hal__msg__HalBatteryMsg(
  const void * untyped_ros_message,
  size_t current_alignment)
{
  const _HalBatteryMsg__ros_msg_type * ros_message = static_cast<const _HalBatteryMsg__ros_msg_type *>(untyped_ros_message);
  (void)ros_message;
  size_t initial_alignment = current_alignment;

  const size_t padding = 4;
  const size_t wchar_size = 4;
  (void)padding;
  (void)wchar_size;

  // field.name battery_status
  {
    size_t item_size = sizeof(ros_message->battery_status);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }
  // field.name battery_current
  {
    size_t item_size = sizeof(ros_message->battery_current);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }
  // field.name cycle_count
  {
    size_t item_size = sizeof(ros_message->cycle_count);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }
  // field.name remain_capacity
  {
    size_t item_size = sizeof(ros_message->remain_capacity);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }
  // field.name total_capacity
  {
    size_t item_size = sizeof(ros_message->total_capacity);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }
  // field.name switch_state
  {
    size_t item_size = sizeof(ros_message->switch_state);
    current_alignment += item_size +
      eprosima::fastcdr::Cdr::alignment(current_alignment, item_size);
  }

  return current_alignment - initial_alignment;
}

static uint32_t _HalBatteryMsg__get_serialized_size(const void * untyped_ros_message)
{
  return static_cast<uint32_t>(
    get_serialized_size_hal__msg__HalBatteryMsg(
      untyped_ros_message, 0));
}

ROSIDL_TYPESUPPORT_FASTRTPS_C_PUBLIC_hal
size_t max_serialized_size_hal__msg__HalBatteryMsg(
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

  // member: battery_status
  {
    size_t array_size = 1;

    last_member_size = array_size * sizeof(uint8_t);
    current_alignment += array_size * sizeof(uint8_t);
  }
  // member: battery_current
  {
    size_t array_size = 1;

    last_member_size = array_size * sizeof(uint16_t);
    current_alignment += array_size * sizeof(uint16_t) +
      eprosima::fastcdr::Cdr::alignment(current_alignment, sizeof(uint16_t));
  }
  // member: cycle_count
  {
    size_t array_size = 1;

    last_member_size = array_size * sizeof(uint16_t);
    current_alignment += array_size * sizeof(uint16_t) +
      eprosima::fastcdr::Cdr::alignment(current_alignment, sizeof(uint16_t));
  }
  // member: remain_capacity
  {
    size_t array_size = 1;

    last_member_size = array_size * sizeof(uint16_t);
    current_alignment += array_size * sizeof(uint16_t) +
      eprosima::fastcdr::Cdr::alignment(current_alignment, sizeof(uint16_t));
  }
  // member: total_capacity
  {
    size_t array_size = 1;

    last_member_size = array_size * sizeof(uint16_t);
    current_alignment += array_size * sizeof(uint16_t) +
      eprosima::fastcdr::Cdr::alignment(current_alignment, sizeof(uint16_t));
  }
  // member: switch_state
  {
    size_t array_size = 1;

    last_member_size = array_size * sizeof(uint8_t);
    current_alignment += array_size * sizeof(uint8_t);
  }

  size_t ret_val = current_alignment - initial_alignment;
  if (is_plain) {
    // All members are plain, and type is not empty.
    // We still need to check that the in-memory alignment
    // is the same as the CDR mandated alignment.
    using DataType = hal__msg__HalBatteryMsg;
    is_plain =
      (
      offsetof(DataType, switch_state) +
      last_member_size
      ) == ret_val;
  }

  return ret_val;
}

static size_t _HalBatteryMsg__max_serialized_size(char & bounds_info)
{
  bool full_bounded;
  bool is_plain;
  size_t ret_val;

  ret_val = max_serialized_size_hal__msg__HalBatteryMsg(
    full_bounded, is_plain, 0);

  bounds_info =
    is_plain ? ROSIDL_TYPESUPPORT_FASTRTPS_PLAIN_TYPE :
    full_bounded ? ROSIDL_TYPESUPPORT_FASTRTPS_BOUNDED_TYPE : ROSIDL_TYPESUPPORT_FASTRTPS_UNBOUNDED_TYPE;
  return ret_val;
}


static message_type_support_callbacks_t __callbacks_HalBatteryMsg = {
  "hal::msg",
  "HalBatteryMsg",
  _HalBatteryMsg__cdr_serialize,
  _HalBatteryMsg__cdr_deserialize,
  _HalBatteryMsg__get_serialized_size,
  _HalBatteryMsg__max_serialized_size
};

static rosidl_message_type_support_t _HalBatteryMsg__type_support = {
  rosidl_typesupport_fastrtps_c__identifier,
  &__callbacks_HalBatteryMsg,
  get_message_typesupport_handle_function,
};

const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_fastrtps_c, hal, msg, HalBatteryMsg)() {
  return &_HalBatteryMsg__type_support;
}

#if defined(__cplusplus)
}
#endif
