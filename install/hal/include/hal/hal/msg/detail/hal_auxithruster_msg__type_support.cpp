// generated from rosidl_typesupport_introspection_cpp/resource/idl__type_support.cpp.em
// with input from hal:msg/HalAuxithrusterMsg.idl
// generated code does not contain a copyright notice

#include "array"
#include "cstddef"
#include "string"
#include "vector"
#include "rosidl_runtime_c/message_type_support_struct.h"
#include "rosidl_typesupport_cpp/message_type_support.hpp"
#include "rosidl_typesupport_interface/macros.h"
#include "hal/msg/detail/hal_auxithruster_msg__struct.hpp"
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

void HalAuxithrusterMsg_init_function(
  void * message_memory, rosidl_runtime_cpp::MessageInitialization _init)
{
  new (message_memory) hal::msg::HalAuxithrusterMsg(_init);
}

void HalAuxithrusterMsg_fini_function(void * message_memory)
{
  auto typed_message = static_cast<hal::msg::HalAuxithrusterMsg *>(message_memory);
  typed_message->~HalAuxithrusterMsg();
}

size_t size_function__HalAuxithrusterMsg__rpm(const void * untyped_member)
{
  (void)untyped_member;
  return 6;
}

const void * get_const_function__HalAuxithrusterMsg__rpm(const void * untyped_member, size_t index)
{
  const auto & member =
    *reinterpret_cast<const std::array<int16_t, 6> *>(untyped_member);
  return &member[index];
}

void * get_function__HalAuxithrusterMsg__rpm(void * untyped_member, size_t index)
{
  auto & member =
    *reinterpret_cast<std::array<int16_t, 6> *>(untyped_member);
  return &member[index];
}

void fetch_function__HalAuxithrusterMsg__rpm(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const auto & item = *reinterpret_cast<const int16_t *>(
    get_const_function__HalAuxithrusterMsg__rpm(untyped_member, index));
  auto & value = *reinterpret_cast<int16_t *>(untyped_value);
  value = item;
}

void assign_function__HalAuxithrusterMsg__rpm(
  void * untyped_member, size_t index, const void * untyped_value)
{
  auto & item = *reinterpret_cast<int16_t *>(
    get_function__HalAuxithrusterMsg__rpm(untyped_member, index));
  const auto & value = *reinterpret_cast<const int16_t *>(untyped_value);
  item = value;
}

size_t size_function__HalAuxithrusterMsg__current(const void * untyped_member)
{
  (void)untyped_member;
  return 6;
}

const void * get_const_function__HalAuxithrusterMsg__current(const void * untyped_member, size_t index)
{
  const auto & member =
    *reinterpret_cast<const std::array<int16_t, 6> *>(untyped_member);
  return &member[index];
}

void * get_function__HalAuxithrusterMsg__current(void * untyped_member, size_t index)
{
  auto & member =
    *reinterpret_cast<std::array<int16_t, 6> *>(untyped_member);
  return &member[index];
}

void fetch_function__HalAuxithrusterMsg__current(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const auto & item = *reinterpret_cast<const int16_t *>(
    get_const_function__HalAuxithrusterMsg__current(untyped_member, index));
  auto & value = *reinterpret_cast<int16_t *>(untyped_value);
  value = item;
}

void assign_function__HalAuxithrusterMsg__current(
  void * untyped_member, size_t index, const void * untyped_value)
{
  auto & item = *reinterpret_cast<int16_t *>(
    get_function__HalAuxithrusterMsg__current(untyped_member, index));
  const auto & value = *reinterpret_cast<const int16_t *>(untyped_value);
  item = value;
}

size_t size_function__HalAuxithrusterMsg__voltage(const void * untyped_member)
{
  (void)untyped_member;
  return 6;
}

const void * get_const_function__HalAuxithrusterMsg__voltage(const void * untyped_member, size_t index)
{
  const auto & member =
    *reinterpret_cast<const std::array<int16_t, 6> *>(untyped_member);
  return &member[index];
}

void * get_function__HalAuxithrusterMsg__voltage(void * untyped_member, size_t index)
{
  auto & member =
    *reinterpret_cast<std::array<int16_t, 6> *>(untyped_member);
  return &member[index];
}

void fetch_function__HalAuxithrusterMsg__voltage(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const auto & item = *reinterpret_cast<const int16_t *>(
    get_const_function__HalAuxithrusterMsg__voltage(untyped_member, index));
  auto & value = *reinterpret_cast<int16_t *>(untyped_value);
  value = item;
}

void assign_function__HalAuxithrusterMsg__voltage(
  void * untyped_member, size_t index, const void * untyped_value)
{
  auto & item = *reinterpret_cast<int16_t *>(
    get_function__HalAuxithrusterMsg__voltage(untyped_member, index));
  const auto & value = *reinterpret_cast<const int16_t *>(untyped_value);
  item = value;
}

size_t size_function__HalAuxithrusterMsg__temp(const void * untyped_member)
{
  (void)untyped_member;
  return 6;
}

const void * get_const_function__HalAuxithrusterMsg__temp(const void * untyped_member, size_t index)
{
  const auto & member =
    *reinterpret_cast<const std::array<uint16_t, 6> *>(untyped_member);
  return &member[index];
}

void * get_function__HalAuxithrusterMsg__temp(void * untyped_member, size_t index)
{
  auto & member =
    *reinterpret_cast<std::array<uint16_t, 6> *>(untyped_member);
  return &member[index];
}

void fetch_function__HalAuxithrusterMsg__temp(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const auto & item = *reinterpret_cast<const uint16_t *>(
    get_const_function__HalAuxithrusterMsg__temp(untyped_member, index));
  auto & value = *reinterpret_cast<uint16_t *>(untyped_value);
  value = item;
}

void assign_function__HalAuxithrusterMsg__temp(
  void * untyped_member, size_t index, const void * untyped_value)
{
  auto & item = *reinterpret_cast<uint16_t *>(
    get_function__HalAuxithrusterMsg__temp(untyped_member, index));
  const auto & value = *reinterpret_cast<const uint16_t *>(untyped_value);
  item = value;
}

size_t size_function__HalAuxithrusterMsg__esc_status(const void * untyped_member)
{
  (void)untyped_member;
  return 6;
}

const void * get_const_function__HalAuxithrusterMsg__esc_status(const void * untyped_member, size_t index)
{
  const auto & member =
    *reinterpret_cast<const std::array<uint8_t, 6> *>(untyped_member);
  return &member[index];
}

void * get_function__HalAuxithrusterMsg__esc_status(void * untyped_member, size_t index)
{
  auto & member =
    *reinterpret_cast<std::array<uint8_t, 6> *>(untyped_member);
  return &member[index];
}

void fetch_function__HalAuxithrusterMsg__esc_status(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const auto & item = *reinterpret_cast<const uint8_t *>(
    get_const_function__HalAuxithrusterMsg__esc_status(untyped_member, index));
  auto & value = *reinterpret_cast<uint8_t *>(untyped_value);
  value = item;
}

void assign_function__HalAuxithrusterMsg__esc_status(
  void * untyped_member, size_t index, const void * untyped_value)
{
  auto & item = *reinterpret_cast<uint8_t *>(
    get_function__HalAuxithrusterMsg__esc_status(untyped_member, index));
  const auto & value = *reinterpret_cast<const uint8_t *>(untyped_value);
  item = value;
}

size_t size_function__HalAuxithrusterMsg__fault_status(const void * untyped_member)
{
  (void)untyped_member;
  return 6;
}

const void * get_const_function__HalAuxithrusterMsg__fault_status(const void * untyped_member, size_t index)
{
  const auto & member =
    *reinterpret_cast<const std::array<uint8_t, 6> *>(untyped_member);
  return &member[index];
}

void * get_function__HalAuxithrusterMsg__fault_status(void * untyped_member, size_t index)
{
  auto & member =
    *reinterpret_cast<std::array<uint8_t, 6> *>(untyped_member);
  return &member[index];
}

void fetch_function__HalAuxithrusterMsg__fault_status(
  const void * untyped_member, size_t index, void * untyped_value)
{
  const auto & item = *reinterpret_cast<const uint8_t *>(
    get_const_function__HalAuxithrusterMsg__fault_status(untyped_member, index));
  auto & value = *reinterpret_cast<uint8_t *>(untyped_value);
  value = item;
}

void assign_function__HalAuxithrusterMsg__fault_status(
  void * untyped_member, size_t index, const void * untyped_value)
{
  auto & item = *reinterpret_cast<uint8_t *>(
    get_function__HalAuxithrusterMsg__fault_status(untyped_member, index));
  const auto & value = *reinterpret_cast<const uint8_t *>(untyped_value);
  item = value;
}

static const ::rosidl_typesupport_introspection_cpp::MessageMember HalAuxithrusterMsg_message_member_array[6] = {
  {
    "rpm",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    true,  // is array
    6,  // array size
    false,  // is upper bound
    offsetof(hal::msg::HalAuxithrusterMsg, rpm),  // bytes offset in struct
    nullptr,  // default value
    size_function__HalAuxithrusterMsg__rpm,  // size() function pointer
    get_const_function__HalAuxithrusterMsg__rpm,  // get_const(index) function pointer
    get_function__HalAuxithrusterMsg__rpm,  // get(index) function pointer
    fetch_function__HalAuxithrusterMsg__rpm,  // fetch(index, &value) function pointer
    assign_function__HalAuxithrusterMsg__rpm,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  },
  {
    "current",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    true,  // is array
    6,  // array size
    false,  // is upper bound
    offsetof(hal::msg::HalAuxithrusterMsg, current),  // bytes offset in struct
    nullptr,  // default value
    size_function__HalAuxithrusterMsg__current,  // size() function pointer
    get_const_function__HalAuxithrusterMsg__current,  // get_const(index) function pointer
    get_function__HalAuxithrusterMsg__current,  // get(index) function pointer
    fetch_function__HalAuxithrusterMsg__current,  // fetch(index, &value) function pointer
    assign_function__HalAuxithrusterMsg__current,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  },
  {
    "voltage",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_INT16,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    true,  // is array
    6,  // array size
    false,  // is upper bound
    offsetof(hal::msg::HalAuxithrusterMsg, voltage),  // bytes offset in struct
    nullptr,  // default value
    size_function__HalAuxithrusterMsg__voltage,  // size() function pointer
    get_const_function__HalAuxithrusterMsg__voltage,  // get_const(index) function pointer
    get_function__HalAuxithrusterMsg__voltage,  // get(index) function pointer
    fetch_function__HalAuxithrusterMsg__voltage,  // fetch(index, &value) function pointer
    assign_function__HalAuxithrusterMsg__voltage,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  },
  {
    "temp",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT16,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    true,  // is array
    6,  // array size
    false,  // is upper bound
    offsetof(hal::msg::HalAuxithrusterMsg, temp),  // bytes offset in struct
    nullptr,  // default value
    size_function__HalAuxithrusterMsg__temp,  // size() function pointer
    get_const_function__HalAuxithrusterMsg__temp,  // get_const(index) function pointer
    get_function__HalAuxithrusterMsg__temp,  // get(index) function pointer
    fetch_function__HalAuxithrusterMsg__temp,  // fetch(index, &value) function pointer
    assign_function__HalAuxithrusterMsg__temp,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  },
  {
    "esc_status",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    true,  // is array
    6,  // array size
    false,  // is upper bound
    offsetof(hal::msg::HalAuxithrusterMsg, esc_status),  // bytes offset in struct
    nullptr,  // default value
    size_function__HalAuxithrusterMsg__esc_status,  // size() function pointer
    get_const_function__HalAuxithrusterMsg__esc_status,  // get_const(index) function pointer
    get_function__HalAuxithrusterMsg__esc_status,  // get(index) function pointer
    fetch_function__HalAuxithrusterMsg__esc_status,  // fetch(index, &value) function pointer
    assign_function__HalAuxithrusterMsg__esc_status,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  },
  {
    "fault_status",  // name
    ::rosidl_typesupport_introspection_cpp::ROS_TYPE_UINT8,  // type
    0,  // upper bound of string
    nullptr,  // members of sub message
    true,  // is array
    6,  // array size
    false,  // is upper bound
    offsetof(hal::msg::HalAuxithrusterMsg, fault_status),  // bytes offset in struct
    nullptr,  // default value
    size_function__HalAuxithrusterMsg__fault_status,  // size() function pointer
    get_const_function__HalAuxithrusterMsg__fault_status,  // get_const(index) function pointer
    get_function__HalAuxithrusterMsg__fault_status,  // get(index) function pointer
    fetch_function__HalAuxithrusterMsg__fault_status,  // fetch(index, &value) function pointer
    assign_function__HalAuxithrusterMsg__fault_status,  // assign(index, value) function pointer
    nullptr  // resize(index) function pointer
  }
};

static const ::rosidl_typesupport_introspection_cpp::MessageMembers HalAuxithrusterMsg_message_members = {
  "hal::msg",  // message namespace
  "HalAuxithrusterMsg",  // message name
  6,  // number of fields
  sizeof(hal::msg::HalAuxithrusterMsg),
  HalAuxithrusterMsg_message_member_array,  // message members
  HalAuxithrusterMsg_init_function,  // function to initialize message memory (memory has to be allocated)
  HalAuxithrusterMsg_fini_function  // function to terminate message instance (will not free memory)
};

static const rosidl_message_type_support_t HalAuxithrusterMsg_message_type_support_handle = {
  ::rosidl_typesupport_introspection_cpp::typesupport_identifier,
  &HalAuxithrusterMsg_message_members,
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
get_message_type_support_handle<hal::msg::HalAuxithrusterMsg>()
{
  return &::hal::msg::rosidl_typesupport_introspection_cpp::HalAuxithrusterMsg_message_type_support_handle;
}

}  // namespace rosidl_typesupport_introspection_cpp

#ifdef __cplusplus
extern "C"
{
#endif

ROSIDL_TYPESUPPORT_INTROSPECTION_CPP_PUBLIC
const rosidl_message_type_support_t *
ROSIDL_TYPESUPPORT_INTERFACE__MESSAGE_SYMBOL_NAME(rosidl_typesupport_introspection_cpp, hal, msg, HalAuxithrusterMsg)() {
  return &::hal::msg::rosidl_typesupport_introspection_cpp::HalAuxithrusterMsg_message_type_support_handle;
}

#ifdef __cplusplus
}
#endif
