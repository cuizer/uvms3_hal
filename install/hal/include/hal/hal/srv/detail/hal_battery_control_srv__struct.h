// NOLINT: This file starts with a BOM since it contain non-ASCII characters
// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from hal:srv/HalBatteryControlSrv.idl
// generated code does not contain a copyright notice

#ifndef HAL__SRV__DETAIL__HAL_BATTERY_CONTROL_SRV__STRUCT_H_
#define HAL__SRV__DETAIL__HAL_BATTERY_CONTROL_SRV__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

/// Struct defined in srv/HalBatteryControlSrv in the package hal.
typedef struct hal__srv__HalBatteryControlSrv_Request
{
  /// 请求部分
  /// 控制命令：
  /// 01 = 12V开
  /// 02 = 12V关
  /// 03 = 24V开
  /// 04 = 24V关
  /// 05 = 72V开
  /// 06 = 72V关
  uint8_t command;
} hal__srv__HalBatteryControlSrv_Request;

// Struct for a sequence of hal__srv__HalBatteryControlSrv_Request.
typedef struct hal__srv__HalBatteryControlSrv_Request__Sequence
{
  hal__srv__HalBatteryControlSrv_Request * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} hal__srv__HalBatteryControlSrv_Request__Sequence;


// Constants defined in the message

// Include directives for member types
// Member 'message'
#include "rosidl_runtime_c/string.h"

/// Struct defined in srv/HalBatteryControlSrv in the package hal.
typedef struct hal__srv__HalBatteryControlSrv_Response
{
  bool success;
  rosidl_runtime_c__String message;
} hal__srv__HalBatteryControlSrv_Response;

// Struct for a sequence of hal__srv__HalBatteryControlSrv_Response.
typedef struct hal__srv__HalBatteryControlSrv_Response__Sequence
{
  hal__srv__HalBatteryControlSrv_Response * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} hal__srv__HalBatteryControlSrv_Response__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // HAL__SRV__DETAIL__HAL_BATTERY_CONTROL_SRV__STRUCT_H_
