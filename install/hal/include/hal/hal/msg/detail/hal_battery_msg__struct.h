// NOLINT: This file starts with a BOM since it contain non-ASCII characters
// generated from rosidl_generator_c/resource/idl__struct.h.em
// with input from hal:msg/HalBatteryMsg.idl
// generated code does not contain a copyright notice

#ifndef HAL__MSG__DETAIL__HAL_BATTERY_MSG__STRUCT_H_
#define HAL__MSG__DETAIL__HAL_BATTERY_MSG__STRUCT_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>


// Constants defined in the message

/// Struct defined in msg/HalBatteryMsg in the package hal.
/**
  * 电池状态消息
  * 节点消息命名: hal_battery_msg
  * 消息命名: uvms_battery_data
 */
typedef struct hal__msg__HalBatteryMsg
{
  /// 电池状态
  uint8_t battery_status;
  /// 电池电流 (单位: 0.1A)
  int16_t battery_current;
  /// 循环次数
  uint16_t cycle_count;
  /// 剩余电量 (单位: 0.1AH)
  uint16_t remain_capacity;
  /// 总电量 (单位: 0.1AH)
  uint16_t total_capacity;
  /// 开关状态: 0=关闭, 1=打开
  uint8_t switch_state;
} hal__msg__HalBatteryMsg;

// Struct for a sequence of hal__msg__HalBatteryMsg.
typedef struct hal__msg__HalBatteryMsg__Sequence
{
  hal__msg__HalBatteryMsg * data;
  /// The number of valid items in data
  size_t size;
  /// The number of allocated items in data
  size_t capacity;
} hal__msg__HalBatteryMsg__Sequence;

#ifdef __cplusplus
}
#endif

#endif  // HAL__MSG__DETAIL__HAL_BATTERY_MSG__STRUCT_H_
