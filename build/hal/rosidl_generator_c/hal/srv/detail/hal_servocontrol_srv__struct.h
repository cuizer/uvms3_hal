// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from hal:srv/HalServocontrolSrv.idl
// generated code does not contain a copyright notice

#ifndef HAL__SRV__DETAIL__HAL_SERVOCONTROL_SRV__STRUCT_H_
#define HAL__SRV__DETAIL__HAL_SERVOCONTROL_SRV__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

/// Struct defined in srv/HalServocontrolSrv in the package hal.
typedef struct hal__srv__HalServocontrolSrv_Request
{
  uint8_t command;
} hal__srv__HalServocontrolSrv_Request;

// Struct for a sequence of hal__srv__HalServocontrolSrv_Request.
typedef struct hal__srv__HalServocontrolSrv_Request__Sequence
{
  hal__srv__HalServocontrolSrv_Request * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} hal__srv__HalServocontrolSrv_Request__Sequence;


// Constants defined in the message

// Include directives for member types
// Member 'message'
#include "rosidl_runtime_c/string.h"

/// Struct defined in srv/HalServocontrolSrv in the package hal.
typedef struct hal__srv__HalServocontrolSrv_Response
{
  bool success;
  rosidl_runtime_c__String message;
} hal__srv__HalServocontrolSrv_Response;

// Struct for a sequence of hal__srv__HalServocontrolSrv_Response.
typedef struct hal__srv__HalServocontrolSrv_Response__Sequence
{
  hal__srv__HalServocontrolSrv_Response * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} hal__srv__HalServocontrolSrv_Response__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // HAL__SRV__DETAIL__HAL_SERVOCONTROL_SRV__STRUCT_H_
