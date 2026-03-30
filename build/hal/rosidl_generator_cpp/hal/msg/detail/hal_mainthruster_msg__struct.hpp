// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from hal:msg/HalMainthrusterMsg.idl
// generated code does not contain a copyright notice

#ifndef HAL__MSG__DETAIL__HAL_MAINTHRUSTER_MSG__STRUCT_HPP_
#define HAL__MSG__DETAIL__HAL_MAINTHRUSTER_MSG__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__hal__msg__HalMainthrusterMsg __attribute__((deprecated))
#else
# define DEPRECATED__hal__msg__HalMainthrusterMsg __declspec(deprecated)
#endif

namespace hal
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct HalMainthrusterMsg_
{
  using Type = HalMainthrusterMsg_<ContainerAllocator>;

  explicit HalMainthrusterMsg_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->rpm = 0;
      this->current = 0;
      this->voltage = 0;
      this->fault_status = 0;
    }
  }

  explicit HalMainthrusterMsg_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    (void)_alloc;
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->rpm = 0;
      this->current = 0;
      this->voltage = 0;
      this->fault_status = 0;
    }
  }

  // field types and members
  using _rpm_type =
    int16_t;
  _rpm_type rpm;
  using _current_type =
    int16_t;
  _current_type current;
  using _voltage_type =
    int16_t;
  _voltage_type voltage;
  using _fault_status_type =
    uint8_t;
  _fault_status_type fault_status;

  // setters for named parameter idiom
  Type & set__rpm(
    const int16_t & _arg)
  {
    this->rpm = _arg;
    return *this;
  }
  Type & set__current(
    const int16_t & _arg)
  {
    this->current = _arg;
    return *this;
  }
  Type & set__voltage(
    const int16_t & _arg)
  {
    this->voltage = _arg;
    return *this;
  }
  Type & set__fault_status(
    const uint8_t & _arg)
  {
    this->fault_status = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    hal::msg::HalMainthrusterMsg_<ContainerAllocator> *;
  using ConstRawPtr =
    const hal::msg::HalMainthrusterMsg_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<hal::msg::HalMainthrusterMsg_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<hal::msg::HalMainthrusterMsg_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      hal::msg::HalMainthrusterMsg_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<hal::msg::HalMainthrusterMsg_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      hal::msg::HalMainthrusterMsg_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<hal::msg::HalMainthrusterMsg_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<hal::msg::HalMainthrusterMsg_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<hal::msg::HalMainthrusterMsg_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__hal__msg__HalMainthrusterMsg
    std::shared_ptr<hal::msg::HalMainthrusterMsg_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__hal__msg__HalMainthrusterMsg
    std::shared_ptr<hal::msg::HalMainthrusterMsg_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const HalMainthrusterMsg_ & other) const
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
    if (this->fault_status != other.fault_status) {
      return false;
    }
    return true;
  }
  bool operator!=(const HalMainthrusterMsg_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct HalMainthrusterMsg_

// alias to use template instance with default allocator
using HalMainthrusterMsg =
  hal::msg::HalMainthrusterMsg_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace hal

#endif  // HAL__MSG__DETAIL__HAL_MAINTHRUSTER_MSG__STRUCT_HPP_
