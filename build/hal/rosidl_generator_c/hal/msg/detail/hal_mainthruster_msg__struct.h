// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from hal:msg/HalMainthrusterMsg.idl
// generated code does not contain a copyright notice

#ifndef HAL__MSG__DETAIL__HAL_MAINTHRUSTER_MSG__STRUCT_H_
#define HAL__MSG__DETAIL__HAL_MAINTHRUSTER_MSG__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

/// Struct defined in msg/HalMainthrusterMsg in the package hal.
typedef struct hal__msg__HalMainthrusterMsg
{
  int16_t rpm;
  int16_t current;
  int16_t voltage;
  uint8_t fault_status;
} hal__msg__HalMainthrusterMsg;

// Struct for a sequence of hal__msg__HalMainthrusterMsg.
typedef struct hal__msg__HalMainthrusterMsg__Sequence
{
  hal__msg__HalMainthrusterMsg * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} hal__msg__HalMainthrusterMsg__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // HAL__MSG__DETAIL__HAL_MAINTHRUSTER_MSG__STRUCT_H_
