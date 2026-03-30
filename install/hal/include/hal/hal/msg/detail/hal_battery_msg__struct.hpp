// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from hal:msg/HalBatteryMsg.idl
// generated code does not contain a copyright notice

#ifndef HAL__MSG__DETAIL__HAL_BATTERY_MSG__STRUCT_HPP_
#define HAL__MSG__DETAIL__HAL_BATTERY_MSG__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__hal__msg__HalBatteryMsg __attribute__((deprecated))
#else
# define DEPRECATED__hal__msg__HalBatteryMsg __declspec(deprecated)
#endif

namespace hal
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct HalBatteryMsg_
{
  using Type = HalBatteryMsg_<ContainerAllocator>;

  explicit HalBatteryMsg_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->battery_status = 0;
      this->battery_current = 0;
      this->cycle_count = 0;
      this->remain_capacity = 0;
      this->total_capacity = 0;
      this->switch_state = 0;
    }
  }

  explicit HalBatteryMsg_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    (void)_alloc;
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      this->battery_status = 0;
      this->battery_current = 0;
      this->cycle_count = 0;
      this->remain_capacity = 0;
      this->total_capacity = 0;
      this->switch_state = 0;
    }
  }

  // field types and members
  using _battery_status_type =
    unsigned char;
  _battery_status_type battery_status;
  using _battery_current_type =
    int16_t;
  _battery_current_type battery_current;
  using _cycle_count_type =
    uint16_t;
  _cycle_count_type cycle_count;
  using _remain_capacity_type =
    uint16_t;
  _remain_capacity_type remain_capacity;
  using _total_capacity_type =
    uint16_t;
  _total_capacity_type total_capacity;
  using _switch_state_type =
    unsigned char;
  _switch_state_type switch_state;

  // setters for named parameter idiom
  Type & set__battery_status(
    const unsigned char & _arg)
  {
    this->battery_status = _arg;
    return *this;
  }
  Type & set__battery_current(
    const int16_t & _arg)
  {
    this->battery_current = _arg;
    return *this;
  }
  Type & set__cycle_count(
    const uint16_t & _arg)
  {
    this->cycle_count = _arg;
    return *this;
  }
  Type & set__remain_capacity(
    const uint16_t & _arg)
  {
    this->remain_capacity = _arg;
    return *this;
  }
  Type & set__total_capacity(
    const uint16_t & _arg)
  {
    this->total_capacity = _arg;
    return *this;
  }
  Type & set__switch_state(
    const unsigned char & _arg)
  {
    this->switch_state = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    hal::msg::HalBatteryMsg_<ContainerAllocator> *;
  using ConstRawPtr =
    const hal::msg::HalBatteryMsg_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<hal::msg::HalBatteryMsg_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<hal::msg::HalBatteryMsg_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      hal::msg::HalBatteryMsg_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<hal::msg::HalBatteryMsg_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      hal::msg::HalBatteryMsg_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<hal::msg::HalBatteryMsg_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<hal::msg::HalBatteryMsg_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<hal::msg::HalBatteryMsg_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__hal__msg__HalBatteryMsg
    std::shared_ptr<hal::msg::HalBatteryMsg_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__hal__msg__HalBatteryMsg
    std::shared_ptr<hal::msg::HalBatteryMsg_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const HalBatteryMsg_ & other) const
  {
    if (this->battery_status != other.battery_status) {
      return false;
    }
    if (this->battery_current != other.battery_current) {
      return false;
    }
    if (this->cycle_count != other.cycle_count) {
      return false;
    }
    if (this->remain_capacity != other.remain_capacity) {
      return false;
    }
    if (this->total_capacity != other.total_capacity) {
      return false;
    }
    if (this->switch_state != other.switch_state) {
      return false;
    }
    return true;
  }
  bool operator!=(const HalBatteryMsg_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct HalBatteryMsg_

// alias to use template instance with default allocator
using HalBatteryMsg =
  hal::msg::HalBatteryMsg_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace hal

#endif  // HAL__MSG__DETAIL__HAL_BATTERY_MSG__STRUCT_HPP_
