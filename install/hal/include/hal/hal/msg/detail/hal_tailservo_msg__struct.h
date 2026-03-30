// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from hal:msg/HalTailservoMsg.idl
// generated code does not contain a copyright notice

#ifndef HAL__MSG__DETAIL__HAL_TAILSERVO_MSG__STRUCT_H_
#define HAL__MSG__DETAIL__HAL_TAILSERVO_MSG__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

/// Struct defined in msg/HalTailservoMsg in the package hal.
typedef struct hal__msg__HalTailservoMsg
{
  int16_t voltage[4];
  int16_t current[4];
  uint16_t power[4];
  uint16_t temperature[4];
  uint8_t status[4];
} hal__msg__HalTailservoMsg;

// Struct for a sequence of hal__msg__HalTailservoMsg.
typedef struct hal__msg__HalTailservoMsg__Sequence
{
  hal__msg__HalTailservoMsg * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} hal__msg__HalTailservoMsg__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // HAL__MSG__DETAIL__HAL_TAILSERVO_MSG__STRUCT_H_
