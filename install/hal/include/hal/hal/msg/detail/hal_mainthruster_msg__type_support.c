// generated from rosidl_typesupport_introspection_c/resource/idl__type_support.c.em
// with input from hal:msg/HalMainthrusterMsg.idl
// generated code does not contain a copyright notice

#include <stddef.h>
#include "hal/msg/detail/hal_mainthruster_msg__rosidl_typesupport_introspection_c.h"
#include "hal/msg/rosidl_typesupport_introspection_c__visibility_control.h"
#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "hal/msg/detail/hal_mainthruster_msg__functions.h"
#include "hal/msg/detail/hal_mainthruster_msg__struct.h"


#ifdef __cplusplus
extern "C"
{
#endif

void hal__msg__HalMainthrusterMsg__rosidl_typesupport_introspection_c__HalMainthrusterMsg_init_function(
  void * message_memory, enum rosidl_runtime_c__message_initialization _init)
{
  // TODO(karsten1987): initializers are not yet implemented for typesupport c
  // see https://github.com/ros2/ros2/issues/397
  (void) _init;
  hal__msg__HalMainthrusterMsg__init(message_memory);
}

void hal__msg__HalMainthrusterMsg__rosidl_typesupport_introspection_c__HalMainthrusterMsg_fini_function(void * message_memory)
{
  hal__msg__HalMainthrusterMsg__fini(message_memory);
}

static rosidl_typesupport_introspection_c__MessageMember hal__msg__HalMainthrusterMsg__rosidl_typesupport_introspection_c__HalMainthrusterMsg_message_member_array[4] = {
  {
    "rpm",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_INT16,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(hal__msg__HalMainthrusterMsg, rpm),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "current",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_INT16,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(hal__msg__HalMainthrusterMsg, current),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "voltage",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_INT16,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(hal__msg__HalMainthrusterMsg, voltage),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "fault_status",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT8,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    false,  // is array
    0,  // array size
    false,  // is upper bound
    offsetof(hal__msg__HalMainthrusterMsg, fault_status),  // bytes offset in struct
    NULL,  // default value
    NULL,  // size() function pointer
    NULL,  // get_const(index) function pointer
    NULL,  // get(index) function pointer
    NULL,  // fetch(index, &value) function pointer
    NULL,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  }
};

static const rosidl_typesupport_introspection_c__MessageMembers hal__msg__HalMainthrusterMsg__rosidl_typesupport_introspection_c__HalMainthrusterMsg_message_members = {
  "hal__msg",  // message namespace
  "HalMainthrusterMsg",  // message name
  4,  // number of fields
  sizeof(hal__msg__HalMainthrusterMsg),
  hal__msg__HalMainthrusterMsg__rosidl_typesupport_introspection_c__HalMainthrusterMsg_message_member_array,  // message members
  hal__msg__HalMainthrusterMsg__rosidl_typesupport_introspection_c__HalMainthrusterMsg_init_function,  // function to initialize message memory (memory has to be allocated)
  hal__msg__HalMainthrusterMsg__rosidl_typesupport_introspection_c__HalMainthrusterMsg_fini_function  // function to terminate message instance (will not free memory)
};

// this is not const since it must be initialized on first access
// since C does not allow non-integral compile-time constants
static rosidl_message_type_support_t hal__msg__HalMainthrusterMsg__rosidl_typesupport_introspection_c__HalMainthrusterMsg_message_type_support_handle = {
  0,
  &hal__msg__HalMainthrusterMsg__rosidl_typesupport_introspection_c__HalMainthrusterMsg_message_members,
  get_message_typesupport_handle_function,
};

ROSIDL_TYPESUPPORT_INTROSPECTION_C_EXPORT_hal
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, hal, msg, HalMainthrusterMsg)() {
  if (!hal__msg__HalMainthrusterMsg__rosidl_typesupport_introspection_c__HalMainthrusterMsg_message_type_support_handle.typesupport_identifier) {
    hal__msg__HalMainthrusterMsg__rosidl_typesupport_introspection_c__HalMainthrusterMsg_message_type_support_handle.typesupport_identifier =
      rosidl_typesupport_introspection_c__identifier;
  }
  return &hal__msg__HalMainthrusterMsg__rosidl_typesupport_introspection_c__HalMainthrusterMsg_message_type_support_handle;
}
#ifdef __cplusplus
}
#endif
