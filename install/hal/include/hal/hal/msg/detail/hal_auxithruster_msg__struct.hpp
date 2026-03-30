// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from hal:msg/HalAuxithrusterMsg.idl
// generated code does not contain a copyright notice

#ifndef HAL__MSG__DETAIL__HAL_AUXITHRUSTER_MSG__STRUCT_HPP_
#define HAL__MSG__DETAIL__HAL_AUXITHRUSTER_MSG__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__hal__msg__HalAuxithrusterMsg __attribute__((deprecated))
#else
# define DEPRECATED__hal__msg__HalAuxithrusterMsg __declspec(deprecated)
#endif

namespace hal
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct HalAuxithrusterMsg_
{
  using Type = HalAuxithrusterMsg_<ContainerAllocator>;

  explicit HalAuxithrusterMsg_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      std::fill<typename std::array<int16_t, 6>::iterator, int16_t>(this->rpm.begin(), this->rpm.end(), 0);
      std::fill<typename std::array<int16_t, 6>::iterator, int16_t>(this->current.begin(), this->current.end(), 0);
      std::fill<typename std::array<int16_t, 6>::iterator, int16_t>(this->voltage.begin(), this->voltage.end(), 0);
      std::fill<typename std::array<uint16_t, 6>::iterator, uint16_t>(this->temp.begin(), this->temp.end(), 0);
      std::fill<typename std::array<uint8_t, 6>::iterator, uint8_t>(this->esc_status.begin(), this->esc_status.end(), 0);
      std::fill<typename std::array<uint8_t, 6>::iterator, uint8_t>(this->fault_status.begin(), this->fault_status.end(), 0);
    }
  }

  explicit HalAuxithrusterMsg_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : rpm(_alloc),
    current(_alloc),
    voltage(_alloc),
    temp(_alloc),
    esc_status(_alloc),
    fault_status(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      std::fill<typename std::array<int16_t, 6>::iterator, int16_t>(this->rpm.begin(), this->rpm.end(), 0);
      std::fill<typename std::array<int16_t, 6>::iterator, int16_t>(this->current.begin(), this->current.end(), 0);
      std::fill<typename std::array<int16_t, 6>::iterator, int16_t>(this->voltage.begin(), this->voltage.end(), 0);
      std::fill<typename std::array<uint16_t, 6>::iterator, uint16_t>(this->temp.begin(), this->temp.end(), 0);
      std::fill<typename std::array<uint8_t, 6>::iterator, uint8_t>(this->esc_status.begin(), this->esc_status.end(), 0);
      std::fill<typename std::array<uint8_t, 6>::iterator, uint8_t>(this->fault_status.begin(), this->fault_status.end(), 0);
    }
  }

  // field types and members
  using _rpm_type =
    std::array<int16_t, 6>;
  _rpm_type rpm;
  using _current_type =
    std::array<int16_t, 6>;
  _current_type current;
  using _voltage_type =
    std::array<int16_t, 6>;
  _voltage_type voltage;
  using _temp_type =
    std::array<uint16_t, 6>;
  _temp_type temp;
  using _esc_status_type =
    std::array<uint8_t, 6>;
  _esc_status_type esc_status;
  using _fault_status_type =
    std::array<uint8_t, 6>;
  _fault_status_type fault_status;

  // setters for named parameter idiom
  Type & set__rpm(
    const std::array<int16_t, 6> & _arg)
  {
    this->rpm = _arg;
    return *this;
  }
  Type & set__current(
    const std::array<int16_t, 6> & _arg)
  {
    this->current = _arg;
    return *this;
  }
  Type & set__voltage(
    const std::array<int16_t, 6> & _arg)
  {
    this->voltage = _arg;
    return *this;
  }
  Type & set__temp(
    const std::array<uint16_t, 6> & _arg)
  {
    this->temp = _arg;
    return *this;
  }
  Type & set__esc_status(
    const std::array<uint8_t, 6> & _arg)
  {
    this->esc_status = _arg;
    return *this;
  }
  Type & set__fault_status(
    const std::array<uint8_t, 6> & _arg)
  {
    this->fault_status = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    hal::msg::HalAuxithrusterMsg_<ContainerAllocator> *;
  using ConstRawPtr =
    const hal::msg::HalAuxithrusterMsg_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<hal::msg::HalAuxithrusterMsg_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<hal::msg::HalAuxithrusterMsg_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      hal::msg::HalAuxithrusterMsg_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<hal::msg::HalAuxithrusterMsg_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      hal::msg::HalAuxithrusterMsg_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<hal::msg::HalAuxithrusterMsg_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<hal::msg::HalAuxithrusterMsg_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<hal::msg::HalAuxithrusterMsg_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__hal__msg__HalAuxithrusterMsg
    std::shared_ptr<hal::msg::HalAuxithrusterMsg_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__hal__msg__HalAuxithrusterMsg
    std::shared_ptr<hal::msg::HalAuxithrusterMsg_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const HalAuxithrusterMsg_ & other) const
  {
    if (this->rpm != other.rpm) {
      return false;
    }
    if (this->current != other.current) {
      return false;
    }
    if (this->voltage != other.voltage) {
      return false;
    }
    if (this->temp != other.temp) {
      return false;
    }
    if (this->esc_status != other.esc_status) {
      return false;
    }
    if (this->fault_status != other.fault_status) {
      return false;
    }
    return true;
  }
  bool operator!=(const HalAuxithrusterMsg_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct HalAuxithrusterMsg_

// alias to use template instance with default allocator
using HalAuxithrusterMsg =
  hal::msg::HalAuxithrusterMsg_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace hal

#endif  // HAL__MSG__DETAIL__HAL_AUXITHRUSTER_MSG__STRUCT_HPP_
