// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from hal:msg/HalWingservoMsg.idl
// generated code does not contain a copyright notice

#ifndef HAL__MSG__DETAIL__HAL_WINGSERVO_MSG__STRUCT_H_
#define HAL__MSG__DETAIL__HAL_WINGSERVO_MSG__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

/// Struct defined in msg/HalWingservoMsg in the package hal.
typedef struct hal__msg__HalWingservoMsg
{
  int16_t voltage[2];
  int16_t current[2];
  uint16_t power[2];
  uint16_t temperature[2];
  uint8_t status[2];
} hal__msg__HalWingservoMsg;

// Struct for a sequence of hal__msg__HalWingservoMsg.
typedef struct hal__msg__HalWingservoMsg__Sequence
{
  hal__msg__HalWingservoMsg * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} hal__msg__HalWingservoMsg__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // HAL__MSG__DETAIL__HAL_WINGSERVO_MSG__STRUCT_H_
