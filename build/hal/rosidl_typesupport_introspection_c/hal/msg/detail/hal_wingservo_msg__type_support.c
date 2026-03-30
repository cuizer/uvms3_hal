// generated from rosidl_typesupport_introspection_c/resource/idl__type_support.c.em
// with input from hal:msg/HalWingservoMsg.idl
// generated code does not contain a copyright notice

#include <stddef.h>
#include "hal/msg/detail/hal_wingservo_msg__rosidl_typesupport_introspection_c.h"
#include "hal/msg/rosidl_typesupport_introspection_c__visibility_control.h"
#include "rosidl_typesupport_introspection_c/field_types.h"
#include "rosidl_typesupport_introspection_c/identifier.h"
#include "rosidl_typesupport_introspection_c/message_introspection.h"
#include "hal/msg/detail/hal_wingservo_msg__functions.h"
#include "hal/msg/detail/hal_wingservo_msg__struct.h"


#ifdef __cplusplus
extern "C"
{
#endif

void hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__HalWingservoMsg_init_function(
  void * message_memory, enum rosidl_runtime_c__message_initialization _init)
{
  // TODO(karsten1987): initializers are not yet implemented for typesupport c
  // see https://github.com/ros2/ros2/issues/397
  (void) _init;
  hal__msg__HalWingservoMsg__init(message_memory);
}

void hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__HalWingservoMsg_fini_function(void * message_memory)
{
  hal__msg__HalWingservoMsg__fini(message_memory);
}

size_t hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__size_function__HalWingservoMsg__voltage(
  const void * untyped_member)
{
  (void)untyped_member;
  return 2;
}

const void * hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_const_function__HalWingservoMsg__voltage(
  const void * untyped_member, size_t index)
{
  const int16_t * member =
    (const int16_t *)(untyped_member);
  return &member[index];
}

void * hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_function__HalWingservoMsg__voltage(
  void * untyped_member, size_t index)
{
  int16_t * member =
    (int16_t *)(untyped_member);
  return &member[index];
}

void hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__fetch_function__HalWingservoMsg__voltage(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const int16_t * item =
    ((const int16_t *)
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_const_function__HalWingservoMsg__voltage(untyped_member, index));
  int16_t * value =
    (int16_t *)(untyped_value);
  *value = *item;
}

void hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__assign_function__HalWingservoMsg__voltage(
  void * untyped_member, size_t index, const void * untyped_value)
{
  int16_t * item =
    ((int16_t *)
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_function__HalWingservoMsg__voltage(untyped_member, index));
  const int16_t * value =
    (const int16_t *)(untyped_value);
  *item = *value;
}

size_t hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__size_function__HalWingservoMsg__current(
  const void * untyped_member)
{
  (void)untyped_member;
  return 2;
}

const void * hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_const_function__HalWingservoMsg__current(
  const void * untyped_member, size_t index)
{
  const int16_t * member =
    (const int16_t *)(untyped_member);
  return &member[index];
}

void * hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_function__HalWingservoMsg__current(
  void * untyped_member, size_t index)
{
  int16_t * member =
    (int16_t *)(untyped_member);
  return &member[index];
}

void hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__fetch_function__HalWingservoMsg__current(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const int16_t * item =
    ((const int16_t *)
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_const_function__HalWingservoMsg__current(untyped_member, index));
  int16_t * value =
    (int16_t *)(untyped_value);
  *value = *item;
}

void hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__assign_function__HalWingservoMsg__current(
  void * untyped_member, size_t index, const void * untyped_value)
{
  int16_t * item =
    ((int16_t *)
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_function__HalWingservoMsg__current(untyped_member, index));
  const int16_t * value =
    (const int16_t *)(untyped_value);
  *item = *value;
}

size_t hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__size_function__HalWingservoMsg__power(
  const void * untyped_member)
{
  (void)untyped_member;
  return 2;
}

const void * hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_const_function__HalWingservoMsg__power(
  const void * untyped_member, size_t index)
{
  const uint16_t * member =
    (const uint16_t *)(untyped_member);
  return &member[index];
}

void * hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_function__HalWingservoMsg__power(
  void * untyped_member, size_t index)
{
  uint16_t * member =
    (uint16_t *)(untyped_member);
  return &member[index];
}

void hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__fetch_function__HalWingservoMsg__power(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const uint16_t * item =
    ((const uint16_t *)
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_const_function__HalWingservoMsg__power(untyped_member, index));
  uint16_t * value =
    (uint16_t *)(untyped_value);
  *value = *item;
}

void hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__assign_function__HalWingservoMsg__power(
  void * untyped_member, size_t index, const void * untyped_value)
{
  uint16_t * item =
    ((uint16_t *)
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_function__HalWingservoMsg__power(untyped_member, index));
  const uint16_t * value =
    (const uint16_t *)(untyped_value);
  *item = *value;
}

size_t hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__size_function__HalWingservoMsg__temperature(
  const void * untyped_member)
{
  (void)untyped_member;
  return 2;
}

const void * hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_const_function__HalWingservoMsg__temperature(
  const void * untyped_member, size_t index)
{
  const uint16_t * member =
    (const uint16_t *)(untyped_member);
  return &member[index];
}

void * hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_function__HalWingservoMsg__temperature(
  void * untyped_member, size_t index)
{
  uint16_t * member =
    (uint16_t *)(untyped_member);
  return &member[index];
}

void hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__fetch_function__HalWingservoMsg__temperature(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const uint16_t * item =
    ((const uint16_t *)
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_const_function__HalWingservoMsg__temperature(untyped_member, index));
  uint16_t * value =
    (uint16_t *)(untyped_value);
  *value = *item;
}

void hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__assign_function__HalWingservoMsg__temperature(
  void * untyped_member, size_t index, const void * untyped_value)
{
  uint16_t * item =
    ((uint16_t *)
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_function__HalWingservoMsg__temperature(untyped_member, index));
  const uint16_t * value =
    (const uint16_t *)(untyped_value);
  *item = *value;
}

size_t hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__size_function__HalWingservoMsg__status(
  const void * untyped_member)
{
  (void)untyped_member;
  return 2;
}

const void * hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_const_function__HalWingservoMsg__status(
  const void * untyped_member, size_t index)
{
  const uint8_t * member =
    (const uint8_t *)(untyped_member);
  return &member[index];
}

void * hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_function__HalWingservoMsg__status(
  void * untyped_member, size_t index)
{
  uint8_t * member =
    (uint8_t *)(untyped_member);
  return &member[index];
}

void hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__fetch_function__HalWingservoMsg__status(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const uint8_t * item =
    ((const uint8_t *)
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_const_function__HalWingservoMsg__status(untyped_member, index));
  uint8_t * value =
    (uint8_t *)(untyped_value);
  *value = *item;
}

void hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__assign_function__HalWingservoMsg__status(
  void * untyped_member, size_t index, const void * untyped_value)
{
  uint8_t * item =
    ((uint8_t *)
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_function__HalWingservoMsg__status(untyped_member, index));
  const uint8_t * value =
    (const uint8_t *)(untyped_value);
  *item = *value;
}

static rosidl_typesupport_introspection_c__MessageMember hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__HalWingservoMsg_message_member_array[5] = {
  {
    "voltage",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_INT16,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    true,  // is array
    2,  // array size
    false,  // is upper bound
    offsetof(hal__msg__HalWingservoMsg, voltage),  // bytes offset in struct
    NULL,  // default value
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__size_function__HalWingservoMsg__voltage,  // size() function pointer
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_const_function__HalWingservoMsg__voltage,  // get_const(index) function pointer
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_function__HalWingservoMsg__voltage,  // get(index) function pointer
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__fetch_function__HalWingservoMsg__voltage,  // fetch(index, &value) function pointer
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__assign_function__HalWingservoMsg__voltage,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "current",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_INT16,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    true,  // is array
    2,  // array size
    false,  // is upper bound
    offsetof(hal__msg__HalWingservoMsg, current),  // bytes offset in struct
    NULL,  // default value
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__size_function__HalWingservoMsg__current,  // size() function pointer
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_const_function__HalWingservoMsg__current,  // get_const(index) function pointer
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_function__HalWingservoMsg__current,  // get(index) function pointer
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__fetch_function__HalWingservoMsg__current,  // fetch(index, &value) function pointer
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__assign_function__HalWingservoMsg__current,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "power",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT16,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    true,  // is array
    2,  // array size
    false,  // is upper bound
    offsetof(hal__msg__HalWingservoMsg, power),  // bytes offset in struct
    NULL,  // default value
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__size_function__HalWingservoMsg__power,  // size() function pointer
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_const_function__HalWingservoMsg__power,  // get_const(index) function pointer
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_function__HalWingservoMsg__power,  // get(index) function pointer
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__fetch_function__HalWingservoMsg__power,  // fetch(index, &value) function pointer
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__assign_function__HalWingservoMsg__power,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "temperature",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT16,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    true,  // is array
    2,  // array size
    false,  // is upper bound
    offsetof(hal__msg__HalWingservoMsg, temperature),  // bytes offset in struct
    NULL,  // default value
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__size_function__HalWingservoMsg__temperature,  // size() function pointer
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_const_function__HalWingservoMsg__temperature,  // get_const(index) function pointer
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_function__HalWingservoMsg__temperature,  // get(index) function pointer
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__fetch_function__HalWingservoMsg__temperature,  // fetch(index, &value) function pointer
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__assign_function__HalWingservoMsg__temperature,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  },
  {
    "status",  // name
    rosidl_typesupport_introspection_c__ROS_TYPE_UINT8,  // type
    0,  // upper bound of string
    NULL,  // members of sub message
    true,  // is array
    2,  // array size
    false,  // is upper bound
    offsetof(hal__msg__HalWingservoMsg, status),  // bytes offset in struct
    NULL,  // default value
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__size_function__HalWingservoMsg__status,  // size() function pointer
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_const_function__HalWingservoMsg__status,  // get_const(index) function pointer
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__get_function__HalWingservoMsg__status,  // get(index) function pointer
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__fetch_function__HalWingservoMsg__status,  // fetch(index, &value) function pointer
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__assign_function__HalWingservoMsg__status,  // assign(index, value) function pointer
    NULL  // resize(index) function pointer
  }
};

static const rosidl_typesupport_introspection_c__MessageMembers hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__HalWingservoMsg_message_members = {
  "hal__msg",  // message namespace
  "HalWingservoMsg",  // message name
  5,  // number of fields
  sizeof(hal__msg__HalWingservoMsg),
  hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__HalWingservoMsg_message_member_array,  // message members
  hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__HalWingservoMsg_init_function,  // function to initialize message memory (memory has to be allocated)
  hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__HalWingservoMsg_fini_function  // function to terminate message instance (will not free memory)
};

// this is not const since it must be initialized on first access
// since C does not allow non-integral compile-time constants
static rosidl_message_type_support_t hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__HalWingservoMsg_message_type_support_handle = {
  0,
  &hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__HalWingservoMsg_message_members,
  get_message_typesupport_handle_function,
};

ROSIDL_TYPESUPPORT_INTROSPECTION_C_EXPORT_hal
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_c, hal, msg, HalWingservoMsg)() {
  if (!hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__HalWingservoMsg_message_type_support_handle.typesupport_identifier) {
    hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__HalWingservoMsg_message_type_support_handle.typesupport_identifier =
      rosidl_typesupport_introspection_c__identifier;
  }
  return &hal__msg__HalWingservoMsg__rosidl_typesupport_introspection_c__HalWingservoMsg_message_type_support_handle;
}
#ifdef __cplusplus
}
#endif
