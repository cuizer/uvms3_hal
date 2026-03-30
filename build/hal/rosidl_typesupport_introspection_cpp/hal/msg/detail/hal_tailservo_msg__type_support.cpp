// generated from rosidl_typesupport_introspection_cpp/resource/idl__type_support.cpp.em
// with input from hal:msg/HalTailservoMsg.idl
// generated code does not contain a copyright notice

#include "array"
#include "cstddef"
#include "string"
#include "vector"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_typesupport_cpp/message_type_support.hpp"
#include "rosidl_typesupport_interface/macros.h"
#include "hal/msg/detail/hal_tailservo_msg__struct.hpp"
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

void HalTailservoMsg_init_function(
  void * message_memory, rosidl_runtime_cpp::MessageInitialization _init)
{
  new (message_memory) hal::msg::HalTailservoMsg(_init);
}

void HalTailservoMsg_fini_function(void * message_memory)
{
  auto typed_message = static_cast<hal::msg::HalTailservoMsg *>(message_memory);
  typed_message->~HalTailservoMsg();
}

size_t size_function__HalTailservoMsg__voltage(const void * untyped_member)
{
  (void)untyped_member;
  return 4;
}

const void * get_const_function__HalTailservoMsg__voltage(const void * untyped_member, size_t index)
{
  const auto & member =
    *reinterpret_cast<const std::array<int16_t, 4> *>(untyped_member);
  return &member[index];
}

void * get_function__HalTailservoMsg__voltage(void * untyped_member, size_t index)
{
  auto & member =
    *reinterpret_cast<std::array<int16_t, 4> *>(untyped_member);
  return &member[index];
}

void fetch_function__HalTailservoMsg__voltage(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const auto & item = *reinterpret_cast<const int16_t *>(
    get_const_function__HalTailservoMsg__voltage(untyped_member, index));
  auto & value = *reinterpret_cast<int16_t *>(untyped_value);
  value = item;
}

void assign_function__HalTailservoMsg__voltage(
  void * untyped_member, size_t index, const void * untyped_value)
{
  auto & item = *reinterpret_cast<int16_t *>(
    get_function__HalTailservoMsg__voltage(untyped_member, index));
  const auto & value = *reinterpret_cast<const int16_t *>(untyped_value);
  item = value;
}

size_t size_function__HalTailservoMsg__current(const void * untyped_member)
{
  (void)untyped_member;
  return 4;
}

const void * get_const_function__HalTailservoMsg__current(const void * untyped_member, size_t index)
{
  const auto & member =
    *reinterpret_cast<const std::array<int16_t, 4> *>(untyped_member);
  return &member[index];
}

void * get_function__HalTailservoMsg__current(void * untyped_member, size_t index)
{
  auto & member =
    *reinterpret_cast<std::array<int16_t, 4> *>(untyped_member);
  return &member[index];
}

void fetch_function__HalTailservoMsg__current(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const auto & item = *reinterpret_cast<const int16_t *>(
    get_const_function__HalTailservoMsg__current(untyped_member, index));
  auto & value = *reinterpret_cast<int16_t *>(untyped_value);
  value = item;
}

void assign_function__HalTailservoMsg__current(
  void * untyped_member, size_t index, const void * untyped_value)
{
  auto & item = *reinterpret_cast<int16_t *>(
    get_function__HalTailservoMsg__current(untyped_member, index));
  const auto & value = *reinterpret_cast<const int16_t *>(untyped_value);
  item = value;
}

size_t size_function__HalTailservoMsg__power(const void * untyped_member)
{
  (void)untyped_member;
  return 4;
}

const void * get_const_function__HalTailservoMsg__power(const void * untyped_member, size_t index)
{
  const auto & member =
    *reinterpret_cast<const std::array<uint16_t, 4> *>(untyped_member);
  return &member[index];
}

void * get_function__HalTailservoMsg__power(void * untyped_member, size_t index)
{
  auto & member =
    *reinterpret_cast<std::array<uint16_t, 4> *>(untyped_member);
  return &member[index];
}

void fetch_function__HalTailservoMsg__power(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const auto & item = *reinterpret_cast<const uint16_t *>(
    get_const_function__HalTailservoMsg__power(untyped_member, index));
  auto & value = *reinterpret_cast<uint16_t *>(untyped_value);
  value = item;
}

void assign_function__HalTailservoMsg__power(
  void * untyped_member, size_t index, const void * untyped_value)
{
  auto & item = *reinterpret_cast<uint16_t *>(
    get_function__HalTailservoMsg__power(untyped_member, index));
  const auto & value = *reinterpret_cast<const uint16_t *>(untyped_value);
  item = value;
}

size_t size_function__HalTailservoMsg__temperature(const void * untyped_member)
{
  (void)untyped_member;
  return 4;
}

const void * get_const_function__HalTailservoMsg__temperature(const void * untyped_member, size_t index)
{
  const auto & member =
    *reinterpret_cast<const std::array<uint16_t, 4> *>(untyped_member);
  return &member[index];
}

void * get_function__HalTailservoMsg__temperature(void * untyped_member, size_t index)
{
  auto & member =
    *reinterpret_cast<std::array<uint16_t, 4> *>(untyped_member);
  return &member[index];
}

void fetch_function__HalTailservoMsg__temperature(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const auto & item = *reinterpret_cast<const uint16_t *>(
    get_const_function__HalTailservoMsg__temperature(untyped_member, index));
  auto & value = *reinterpret_cast<uint16_t *>(untyped_value);
  value = item;
}

void assign_function__HalTailservoMsg__temperature(
  void * untyped_member, size_t index, const void * untyped_value)
{
  auto & item = *reinterpret_cast<uint16_t *>(
    get_function__HalTailservoMsg__temperature(untyped_member, index));
  const auto & value = *reinterpret_cast<const uint16_t *>(untyped_value);
  item = value;
}

size_t size_function__HalTailservoMsg__status(const void * untyped_member)
{
  (void)untyped_member;
  return 4;
}

const void * get_const_function__HalTailservoMsg__status(const void * untyped_member, size_t index)
{
  const auto & member =
    *reinterpret_cast<const std::array<uint8_t, 4> *>(untyped_member);
  return &member[index];
}

void * get_function__HalTailservoMsg__status(void * untyped_member, size_t index)
{
  auto & member =
    *reinterpret_cast<std::array<uint8_t, 4> *>(untyped_member);
  return &member[index];
}

void fetch_function__HalTailservoMsg__status(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const auto & item = *reinterpret_cast<const uint8_t *>(
    get_const_function__HalTailservoMsg__status(untyped_member, index));
  auto & value = *reinterpret_cast<uint8_t *>(untyped_value);
  value = item;
}

void assign_function__HalTailservoMsg__status(
  void * untyped_member, size_t index, const void * untyped_value)
{
  auto & item = *reinterpret_cast<uint8_t *>(
    get_function__HalTailservoMsg__status(untyped_member, index));
  const auto & value = *reinterpret_cast<const uint8_t *>(untyped_value);
  item = value;
}

static const ::rosidl_typesupport_introspection_cpp::MessageMember HalTailservoMsg_message_member_array[5] = {
  {
    "voltage",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    true,  // is array
    4,  // array size
    false,  // is upper bound
    offsetof(hal::msg::HalTailservoMsg, voltage),  // bytes offset in struct
    nullptr,  // default value
    size_function__HalTailservoMsg__voltage,  // size() function pointer
    get_const_function__HalTailservoMsg__voltage,  // get_const(index) function pointer
    get_function__HalTailservoMsg__voltage,  // get(index) function pointer
    fetch_function__HalTailservoMsg__voltage,  // fetch(index, &value) function pointer
    assign_function__HalTailservoMsg__voltage,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  },
  {
    "current",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    true,  // is array
    4,  // array size
    false,  // is upper bound
    offsetof(hal::msg::HalTailservoMsg, current),  // bytes offset in struct
    nullptr,  // default value
    size_function__HalTailservoMsg__current,  // size() function pointer
    get_const_function__HalTailservoMsg__current,  // get_const(index) function pointer
    get_function__HalTailservoMsg__current,  // get(index) function pointer
    fetch_function__HalTailservoMsg__current,  // fetch(index, &value) function pointer
    assign_function__HalTailservoMsg__current,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  },
  {
    "power",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    true,  // is array
    4,  // array size
    false,  // is upper bound
    offsetof(hal::msg::HalTailservoMsg, power),  // bytes offset in struct
    nullptr,  // default value
    size_function__HalTailservoMsg__power,  // size() function pointer
    get_const_function__HalTailservoMsg__power,  // get_const(index) function pointer
    get_function__HalTailservoMsg__power,  // get(index) function pointer
    fetch_function__HalTailservoMsg__power,  // fetch(index, &value) function pointer
    assign_function__HalTailservoMsg__power,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  },
  {
    "temperature",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    true,  // is array
    4,  // array size
    false,  // is upper bound
    offsetof(hal::msg::HalTailservoMsg, temperature),  // bytes offset in struct
    nullptr,  // default value
    size_function__HalTailservoMsg__temperature,  // size() function pointer
    get_const_function__HalTailservoMsg__temperature,  // get_const(index) function pointer
    get_function__HalTailservoMsg__temperature,  // get(index) function pointer
    fetch_function__HalTailservoMsg__temperature,  // fetch(index, &value) function pointer
    assign_function__HalTailservoMsg__temperature,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  },
  {
    "status",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    true,  // is array
    4,  // array size
    false,  // is upper bound
    offsetof(hal::msg::HalTailservoMsg, status),  // bytes offset in struct
    nullptr,  // default value
    size_function__HalTailservoMsg__status,  // size() function pointer
    get_const_function__HalTailservoMsg__status,  // get_const(index) function pointer
    get_function__HalTailservoMsg__status,  // get(index) function pointer
    fetch_function__HalTailservoMsg__status,  // fetch(index, &value) function pointer
    assign_function__HalTailservoMsg__status,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  }
};

static const ::rosidl_typesupport_introspection_cpp::MessageMembers HalTailservoMsg_message_members = {
  "hal::msg",  // message namespace
  "HalTailservoMsg",  // message name
  5,  // number of fields
  sizeof(hal::msg::HalTailservoMsg),
  HalTailservoMsg_message_member_array,  // message members
  HalTailservoMsg_init_function,  // function to initialize message memory (memory has to be allocated)
  HalTailservoMsg_fini_function  // function to terminate message instance (will not free memory)
};

static const rosidl_message_type_support_t HalTailservoMsg_message_type_support_handle = {
  ::rosidl_typesupport_introspection_cpp::typesupport_identifier,
  &HalTailservoMsg_message_members,
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
get_message_type_support_handle<hal::msg::HalTailservoMsg>()
{
  return &::hal::msg::rosidl_typesupport_introspection_cpp::HalTailservoMsg_message_type_support_handle;
}

}  // namespace rosidl_typesupport_introspection_cpp

#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_INTROSPECTION_CPP_PUBLIC
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_cpp, hal, msg, HalTailservoMsg)() {
  return &::hal::msg::rosidl_typesupport_introspection_cpp::HalTailservoMsg_message_type_support_handle;
}

#ifdef __cplusplus
}
#endif
