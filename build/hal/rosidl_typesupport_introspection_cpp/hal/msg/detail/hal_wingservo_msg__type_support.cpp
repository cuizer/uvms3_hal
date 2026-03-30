// generated from rosidl_typesupport_introspection_cpp/resource/idl__type_support.cpp.em
// with input from hal:msg/HalWingservoMsg.idl
// generated code does not contain a copyright notice

#include "array"
#include "cstddef"
#include "string"
#include "vector"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_typesupport_cpp/message_type_support.hpp"
#include "rosidl_typesupport_interface/macros.h"
#include "hal/msg/detail/hal_wingservo_msg__struct.hpp"
#include "rosidl_typesupport_introspection_cpp/field_types.hpp"
#include "rosidl_typesupport_introspection_cpp/identifier.hpp"
#include "rosidl_typesupport_introspection_cpp/message_introspection.hpp"
#include "rosidl_typesupport_introspection_cpp/message_type_support_decl.hpp"
#include "rosidl_typesupport_introspection_cpp/visibility_control.h"

namespace hal
{

namespace msg
{

namespace rosidl_typesupport_introspection_cpp
{

void HalWingservoMsg_init_function(
  void * message_memory, rosidl_runtime_cpp::MessageInitialization _init)
{
  new (message_memory) hal::msg::HalWingservoMsg(_init);
}

void HalWingservoMsg_fini_function(void * message_memory)
{
  auto typed_message = static_cast<hal::msg::HalWingservoMsg *>(message_memory);
  typed_message->~HalWingservoMsg();
}

size_t size_function__HalWingservoMsg__voltage(const void * untyped_member)
{
  (void)untyped_member;
  return 2;
}

const void * get_const_function__HalWingservoMsg__voltage(const void * untyped_member, size_t index)
{
  const auto & member =
    *reinterpret_cast<const std::array<int16_t, 2> *>(untyped_member);
  return &member[index];
}

void * get_function__HalWingservoMsg__voltage(void * untyped_member, size_t index)
{
  auto & member =
    *reinterpret_cast<std::array<int16_t, 2> *>(untyped_member);
  return &member[index];
}

void fetch_function__HalWingservoMsg__voltage(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const auto & item = *reinterpret_cast<const int16_t *>(
    get_const_function__HalWingservoMsg__voltage(untyped_member, index));
  auto & value = *reinterpret_cast<int16_t *>(untyped_value);
  value = item;
}

void assign_function__HalWingservoMsg__voltage(
  void * untyped_member, size_t index, const void * untyped_value)
{
  auto & item = *reinterpret_cast<int16_t *>(
    get_function__HalWingservoMsg__voltage(untyped_member, index));
  const auto & value = *reinterpret_cast<const int16_t *>(untyped_value);
  item = value;
}

size_t size_function__HalWingservoMsg__current(const void * untyped_member)
{
  (void)untyped_member;
  return 2;
}

const void * get_const_function__HalWingservoMsg__current(const void * untyped_member, size_t index)
{
  const auto & member =
    *reinterpret_cast<const std::array<int16_t, 2> *>(untyped_member);
  return &member[index];
}

void * get_function__HalWingservoMsg__current(void * untyped_member, size_t index)
{
  auto & member =
    *reinterpret_cast<std::array<int16_t, 2> *>(untyped_member);
  return &member[index];
}

void fetch_function__HalWingservoMsg__current(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const auto & item = *reinterpret_cast<const int16_t *>(
    get_const_function__HalWingservoMsg__current(untyped_member, index));
  auto & value = *reinterpret_cast<int16_t *>(untyped_value);
  value = item;
}

void assign_function__HalWingservoMsg__current(
  void * untyped_member, size_t index, const void * untyped_value)
{
  auto & item = *reinterpret_cast<int16_t *>(
    get_function__HalWingservoMsg__current(untyped_member, index));
  const auto & value = *reinterpret_cast<const int16_t *>(untyped_value);
  item = value;
}

size_t size_function__HalWingservoMsg__power(const void * untyped_member)
{
  (void)untyped_member;
  return 2;
}

const void * get_const_function__HalWingservoMsg__power(const void * untyped_member, size_t index)
{
  const auto & member =
    *reinterpret_cast<const std::array<uint16_t, 2> *>(untyped_member);
  return &member[index];
}

void * get_function__HalWingservoMsg__power(void * untyped_member, size_t index)
{
  auto & member =
    *reinterpret_cast<std::array<uint16_t, 2> *>(untyped_member);
  return &member[index];
}

void fetch_function__HalWingservoMsg__power(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const auto & item = *reinterpret_cast<const uint16_t *>(
    get_const_function__HalWingservoMsg__power(untyped_member, index));
  auto & value = *reinterpret_cast<uint16_t *>(untyped_value);
  value = item;
}

void assign_function__HalWingservoMsg__power(
  void * untyped_member, size_t index, const void * untyped_value)
{
  auto & item = *reinterpret_cast<uint16_t *>(
    get_function__HalWingservoMsg__power(untyped_member, index));
  const auto & value = *reinterpret_cast<const uint16_t *>(untyped_value);
  item = value;
}

size_t size_function__HalWingservoMsg__temperature(const void * untyped_member)
{
  (void)untyped_member;
  return 2;
}

const void * get_const_function__HalWingservoMsg__temperature(const void * untyped_member, size_t index)
{
  const auto & member =
    *reinterpret_cast<const std::array<uint16_t, 2> *>(untyped_member);
  return &member[index];
}

void * get_function__HalWingservoMsg__temperature(void * untyped_member, size_t index)
{
  auto & member =
    *reinterpret_cast<std::array<uint16_t, 2> *>(untyped_member);
  return &member[index];
}

void fetch_function__HalWingservoMsg__temperature(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const auto & item = *reinterpret_cast<const uint16_t *>(
    get_const_function__HalWingservoMsg__temperature(untyped_member, index));
  auto & value = *reinterpret_cast<uint16_t *>(untyped_value);
  value = item;
}

void assign_function__HalWingservoMsg__temperature(
  void * untyped_member, size_t index, const void * untyped_value)
{
  auto & item = *reinterpret_cast<uint16_t *>(
    get_function__HalWingservoMsg__temperature(untyped_member, index));
  const auto & value = *reinterpret_cast<const uint16_t *>(untyped_value);
  item = value;
}

size_t size_function__HalWingservoMsg__status(const void * untyped_member)
{
  (void)untyped_member;
  return 2;
}

const void * get_const_function__HalWingservoMsg__status(const void * untyped_member, size_t index)
{
  const auto & member =
    *reinterpret_cast<const std::array<uint8_t, 2> *>(untyped_member);
  return &member[index];
}

void * get_function__HalWingservoMsg__status(void * untyped_member, size_t index)
{
  auto & member =
    *reinterpret_cast<std::array<uint8_t, 2> *>(untyped_member);
  return &member[index];
}

void fetch_function__HalWingservoMsg__status(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const auto & item = *reinterpret_cast<const uint8_t *>(
    get_const_function__HalWingservoMsg__status(untyped_member, index));
  auto & value = *reinterpret_cast<uint8_t *>(untyped_value);
  value = item;
}

void assign_function__HalWingservoMsg__status(
  void * untyped_member, size_t index, const void * untyped_value)
{
  auto & item = *reinterpret_cast<uint8_t *>(
    get_function__HalWingservoMsg__status(untyped_member, index));
  const auto & value = *reinterpret_cast<const uint8_t *>(untyped_value);
  item = value;
}

static const ::rosidl_typesupport_introspection_cpp::MessageMember HalWingservoMsg_message_member_array[5] = {
  {
    "voltage",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    true,  // is array
    2,  // array size
    false,  // is upper bound
    offsetof(hal::msg::HalWingservoMsg, voltage),  // bytes offset in struct
    nullptr,  // default value
    size_function__HalWingservoMsg__voltage,  // size() function pointer
    get_const_function__HalWingservoMsg__voltage,  // get_const(index) function pointer
    get_function__HalWingservoMsg__voltage,  // get(index) function pointer
    fetch_function__HalWingservoMsg__voltage,  // fetch(index, &value) function pointer
    assign_function__HalWingservoMsg__voltage,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  },
  {
    "current",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    true,  // is array
    2,  // array size
    false,  // is upper bound
    offsetof(hal::msg::HalWingservoMsg, current),  // bytes offset in struct
    nullptr,  // default value
    size_function__HalWingservoMsg__current,  // size() function pointer
    get_const_function__HalWingservoMsg__current,  // get_const(index) function pointer
    get_function__HalWingservoMsg__current,  // get(index) function pointer
    fetch_function__HalWingservoMsg__current,  // fetch(index, &value) function pointer
    assign_function__HalWingservoMsg__current,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  },
  {
    "power",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    true,  // is array
    2,  // array size
    false,  // is upper bound
    offsetof(hal::msg::HalWingservoMsg, power),  // bytes offset in struct
    nullptr,  // default value
    size_function__HalWingservoMsg__power,  // size() function pointer
    get_const_function__HalWingservoMsg__power,  // get_const(index) function pointer
    get_function__HalWingservoMsg__power,  // get(index) function pointer
    fetch_function__HalWingservoMsg__power,  // fetch(index, &value) function pointer
    assign_function__HalWingservoMsg__power,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  },
  {
    "temperature",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    true,  // is array
    2,  // array size
    false,  // is upper bound
    offsetof(hal::msg::HalWingservoMsg, temperature),  // bytes offset in struct
    nullptr,  // default value
    size_function__HalWingservoMsg__temperature,  // size() function pointer
    get_const_function__HalWingservoMsg__temperature,  // get_const(index) function pointer
    get_function__HalWingservoMsg__temperature,  // get(index) function pointer
    fetch_function__HalWingservoMsg__temperature,  // fetch(index, &value) function pointer
    assign_function__HalWingservoMsg__temperature,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  },
  {
    "status",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    true,  // is array
    2,  // array size
    false,  // is upper bound
    offsetof(hal::msg::HalWingservoMsg, status),  // bytes offset in struct
    nullptr,  // default value
    size_function__HalWingservoMsg__status,  // size() function pointer
    get_const_function__HalWingservoMsg__status,  // get_const(index) function pointer
    get_function__HalWingservoMsg__status,  // get(index) function pointer
    fetch_function__HalWingservoMsg__status,  // fetch(index, &value) function pointer
    assign_function__HalWingservoMsg__status,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  }
};

static const ::rosidl_typesupport_introspection_cpp::MessageMembers HalWingservoMsg_message_members = {
  "hal::msg",  // message namespace
  "HalWingservoMsg",  // message name
  5,  // number of fields
  sizeof(hal::msg::HalWingservoMsg),
  HalWingservoMsg_message_member_array,  // message members
  HalWingservoMsg_init_function,  // function to initialize message memory (memory has to be allocated)
  HalWingservoMsg_fini_function  // function to terminate message instance (will not free memory)
};

static const rosidl_message_type_support_t HalWingservoMsg_message_type_support_handle = {
  ::rosidl_typesupport_introspection_cpp::typesupport_identifier,
  &HalWingservoMsg_message_members,
  get_message_typesupport_handle_function,
};

}  // namespace rosidl_typesupport_introspection_cpp

}  // namespace msg

}  // namespace hal


namespace rosidl_typesupport_introspection_cpp
{

template<>
ROSIDL_TYPESUPPORT_INTROSPECTION_CPP_PUBLIC
const rosidl_message_type_support_t *
get_message_type_support_handle<hal::msg::HalWingservoMsg>()
{
  return &::hal::msg::rosidl_typesupport_introspection_cpp::HalWingservoMsg_message_type_support_handle;
}

}  // namespace rosidl_typesupport_introspection_cpp

#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_INTROSPECTION_CPP_PUBLIC
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_cpp, hal, msg, HalWingservoMsg)() {
  return &::hal::msg::rosidl_typesupport_introspection_cpp::HalWingservoMsg_message_type_support_handle;
}

#ifdef __cplusplus
}
#endif
