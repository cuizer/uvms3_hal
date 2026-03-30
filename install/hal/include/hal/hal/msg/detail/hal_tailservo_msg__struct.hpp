// generated from rosidl_generator_cpp/resource/idl__struct.hpp.em
// with input from hal:msg/HalTailservoMsg.idl
// generated code does not contain a copyright notice

#ifndef HAL__MSG__DETAIL__HAL_TAILSERVO_MSG__STRUCT_HPP_
#define HAL__MSG__DETAIL__HAL_TAILSERVO_MSG__STRUCT_HPP_

#include <algorithm>
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "rosidl_runtime_cpp/bounded_vector.hpp"
#include "rosidl_runtime_cpp/message_initialization.hpp"


#ifndef _WIN32
# define DEPRECATED__hal__msg__HalTailservoMsg __attribute__((deprecated))
#else
# define DEPRECATED__hal__msg__HalTailservoMsg __declspec(deprecated)
#endif

namespace hal
{

namespace msg
{

// message struct
template<class ContainerAllocator>
struct HalTailservoMsg_
{
  using Type = HalTailservoMsg_<ContainerAllocator>;

  explicit HalTailservoMsg_(rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      std::fill<typename std::array<int16_t, 4>::iterator, int16_t>(this->voltage.begin(), this->voltage.end(), 0);
      std::fill<typename std::array<int16_t, 4>::iterator, int16_t>(this->current.begin(), this->current.end(), 0);
      std::fill<typename std::array<uint16_t, 4>::iterator, uint16_t>(this->power.begin(), this->power.end(), 0);
      std::fill<typename std::array<uint16_t, 4>::iterator, uint16_t>(this->temperature.begin(), this->temperature.end(), 0);
      std::fill<typename std::array<uint8_t, 4>::iterator, uint8_t>(this->status.begin(), this->status.end(), 0);
    }
  }

  explicit HalTailservoMsg_(const ContainerAllocator & _alloc, rosidl_runtime_cpp::MessageInitialization _init = rosidl_runtime_cpp::MessageInitialization::ALL)
  : voltage(_alloc),
    current(_alloc),
    power(_alloc),
    temperature(_alloc),
    status(_alloc)
  {
    if (rosidl_runtime_cpp::MessageInitialization::ALL == _init ||
      rosidl_runtime_cpp::MessageInitialization::ZERO == _init)
    {
      std::fill<typename std::array<int16_t, 4>::iterator, int16_t>(this->voltage.begin(), this->voltage.end(), 0);
      std::fill<typename std::array<int16_t, 4>::iterator, int16_t>(this->current.begin(), this->current.end(), 0);
      std::fill<typename std::array<uint16_t, 4>::iterator, uint16_t>(this->power.begin(), this->power.end(), 0);
      std::fill<typename std::array<uint16_t, 4>::iterator, uint16_t>(this->temperature.begin(), this->temperature.end(), 0);
      std::fill<typename std::array<uint8_t, 4>::iterator, uint8_t>(this->status.begin(), this->status.end(), 0);
    }
  }

  // field types and members
  using _voltage_type =
    std::array<int16_t, 4>;
  _voltage_type voltage;
  using _current_type =
    std::array<int16_t, 4>;
  _current_type current;
  using _power_type =
    std::array<uint16_t, 4>;
  _power_type power;
  using _temperature_type =
    std::array<uint16_t, 4>;
  _temperature_type temperature;
  using _status_type =
    std::array<uint8_t, 4>;
  _status_type status;

  // setters for named parameter idiom
  Type & set__voltage(
    const std::array<int16_t, 4> & _arg)
  {
    this->voltage = _arg;
    return *this;
  }
  Type & set__current(
    const std::array<int16_t, 4> & _arg)
  {
    this->current = _arg;
    return *this;
  }
  Type & set__power(
    const std::array<uint16_t, 4> & _arg)
  {
    this->power = _arg;
    return *this;
  }
  Type & set__temperature(
    const std::array<uint16_t, 4> & _arg)
  {
    this->temperature = _arg;
    return *this;
  }
  Type & set__status(
    const std::array<uint8_t, 4> & _arg)
  {
    this->status = _arg;
    return *this;
  }

  // constant declarations

  // pointer types
  using RawPtr =
    hal::msg::HalTailservoMsg_<ContainerAllocator> *;
  using ConstRawPtr =
    const hal::msg::HalTailservoMsg_<ContainerAllocator> *;
  using SharedPtr =
    std::shared_ptr<hal::msg::HalTailservoMsg_<ContainerAllocator>>;
  using ConstSharedPtr =
    std::shared_ptr<hal::msg::HalTailservoMsg_<ContainerAllocator> const>;

  template<typename Deleter = std::default_delete<
      hal::msg::HalTailservoMsg_<ContainerAllocator>>>
  using UniquePtrWithDeleter =
    std::unique_ptr<hal::msg::HalTailservoMsg_<ContainerAllocator>, Deleter>;

  using UniquePtr = UniquePtrWithDeleter<>;

  template<typename Deleter = std::default_delete<
      hal::msg::HalTailservoMsg_<ContainerAllocator>>>
  using ConstUniquePtrWithDeleter =
    std::unique_ptr<hal::msg::HalTailservoMsg_<ContainerAllocator> const, Deleter>;
  using ConstUniquePtr = ConstUniquePtrWithDeleter<>;

  using WeakPtr =
    std::weak_ptr<hal::msg::HalTailservoMsg_<ContainerAllocator>>;
  using ConstWeakPtr =
    std::weak_ptr<hal::msg::HalTailservoMsg_<ContainerAllocator> const>;

  // pointer types similar to ROS 1, use SharedPtr / ConstSharedPtr instead
  // NOTE: Can't use 'using' here because GNU C++ can't parse attributes properly
  typedef DEPRECATED__hal__msg__HalTailservoMsg
    std::shared_ptr<hal::msg::HalTailservoMsg_<ContainerAllocator>>
    Ptr;
  typedef DEPRECATED__hal__msg__HalTailservoMsg
    std::shared_ptr<hal::msg::HalTailservoMsg_<ContainerAllocator> const>
    ConstPtr;

  // comparison operators
  bool operator==(const HalTailservoMsg_ & other) const
  {
    if (this->voltage != other.voltage) {
      return false;
    }
    if (this->current != other.current) {
      return false;
    }
    if (this->power != other.power) {
      return false;
    }
    if (this->temperature != other.temperature) {
      return false;
    }
    if (this->status != other.status) {
      return false;
    }
    return true;
  }
  bool operator!=(const HalTailservoMsg_ & other) const
  {
    return !this->operator==(other);
  }
};  // struct HalTailservoMsg_

// alias to use template instance with default allocator
using HalTailservoMsg =
  hal::msg::HalTailservoMsg_<std::allocator<void>>;

// constant definitions

}  // namespace msg

}  // namespace hal

#endif  // HAL__MSG__DETAIL__HAL_TAILSERVO_MSG__STRUCT_HPP_
