// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from hal:msg/HalAuxithrusterMsg.idl
// generated code does not contain a copyright notice

#ifndef HAL__MSG__DETAIL__HAL_AUXITHRUSTER_MSG__STRUCT_H_
#define HAL__MSG__DETAIL__HAL_AUXITHRUSTER_MSG__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

/// Struct defined in msg/HalAuxithrusterMsg in the package hal.
typedef struct hal__msg__HalAuxithrusterMsg
{
  int16_t rpm[6];
  int16_t current[6];
  int16_t voltage[6];
  uint16_t temp[6];
  uint8_t esc_status[6];
  uint8_t fault_status[6];
} hal__msg__HalAuxithrusterMsg;

// Struct for a sequence of hal__msg__HalAuxithrusterMsg.
typedef struct hal__msg__HalAuxithrusterMsg__Sequence
{
  hal__msg__HalAuxithrusterMsg * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} hal__msg__HalAuxithrusterMsg__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // HAL__MSG__DETAIL__HAL_AUXITHRUSTER_MSG__STRUCT_H_
