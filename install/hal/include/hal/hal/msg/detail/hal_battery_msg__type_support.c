// generated from rosidl_typesupport_introspection_c/resource/idl__type_support.c.em
// with input from hal:msg/HalBatteryMsg.idl
// generated code does not contain a copyright notice

#include <stddef.h>
#include "hal/msg/detail/hal_battery_msg__rosidl_typesupport_introspection_c.h"
#include "hal/msg/rosidl_typesupport_introspection_c__visibility_control.h"
#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "hal/msg/detail/hal_battery_msg__functions.h"
#include "hal/msg/detail/hal_battery_msg__struct.h"


#ifdef __cplusplus
extern "C"
{
#endif

void hal__msg__HalBatteryMsg__rosidl_typesupport_introspection_c__HalBatteryMsg_init_function(
  void * message_memory, enum rosidl_runtime_c__message_initialization _init)
{
  // TODO(karsten1987): initializers are not yet implemented for typesupport c
  // see https://github.com/ros2/ros2/issues/397
  (void) _init;
  hal__msg__HalBatteryMsg__init(message_memory);
}

void hal__msg__HalBatteryMsg__rosidl_typesupport_introspection_c__HalBatteryMsg_fini_function(void * message_memory)
{
  hal__msg__HalBatteryMsg__fini(message_memory);
}

static rosidl_typesupport_introspection_c__MessageMember hal__msg__HalBatteryMsg__rosidl_typesupport_introspection_c__HalBatteryMsg_message_member_array[6] = {
  {
    "battery_status",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_OCTET,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(hal__msg__HalBatteryMsg, battery_status),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "battery_current",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_INT16,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(hal__msg__HalBatteryMsg, battery_current),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "cycle_count",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT16,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(hal__msg__HalBatteryMsg, cycle_count),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "remain_capacity",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT16,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(hal__msg__HalBatteryMsg, remain_capacity),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "total_capacity",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT16,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(hal__msg__HalBatteryMsg, total_capacity),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "switch_state",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_OCTET,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(hal__msg__HalBatteryMsg, switch_state),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  }
};

static const rosidl_typesupport_introspection_c__MessageMembers hal__msg__HalBatteryMsg__rosidl_typesupport_introspection_c__HalBatteryMsg_message_members = {
  "hal__msg",  // message namespace
  "HalBatteryMsg",  // message name
  6,  // number of fields
  sizeof(hal__msg__HalBatteryMsg),
  hal__msg__HalBatteryMsg__rosidl_typesupport_introspection_c__HalBatteryMsg_message_member_array,  // message members
  hal__msg__HalBatteryMsg__rosidl_typesupport_introspection_c__HalBatteryMsg_init_function,  // function to initialize message memory (memory has to be allocated)
  hal__msg__HalBatteryMsg__rosidl_typesupport_introspection_c__HalBatteryMsg_fini_function  // function to terminate message instance (will not free memory)
};

// this is not const since it must be initialized on first access
// since C does not allow non-integral compile-time constants
static rosidl_message_type_support_t hal__msg__HalBatteryMsg__rosidl_typesupport_introspection_c__HalBatteryMsg_message_type_support_handle = {
  0,
  &hal__msg__HalBatteryMsg__rosidl_typesupport_introspection_c__HalBatteryMsg_message_members,
  get_message_typesupport_handle_function,
};

ROSIDL_TYPESUPPORT_INTROSPECTION_C_EXPORT_hal
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, hal, msg, HalBatteryMsg)() {
  if (!hal__msg__HalBatteryMsg__rosidl_typesupport_introspection_c__HalBatteryMsg_message_type_support_handle.typesupport_identifier) {
    hal__msg__HalBatteryMsg__rosidl_typesupport_introspection_c__HalBatteryMsg_message_type_support_handle.typesupport_identifier =
      rosidl_typesupport_introspection_c__identifier;
  }
  return &hal__msg__HalBatteryMsg__rosidl_typesupport_introspection_c__HalBatteryMsg_message_type_support_handle;
}
#ifdef __cplusplus
}
#endif
