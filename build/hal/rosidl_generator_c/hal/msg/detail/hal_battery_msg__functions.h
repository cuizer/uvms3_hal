// generated from rosidl_generator_c/resource/idl__functions.h.em
// with input from hal:msg/HalBatteryMsg.idl
// generated code does not contain a copyright notice

#ifndef HAL__MSG__DETAIL__HAL_BATTERY_MSG__FUNCTIONS_H_
#define HAL__MSG__DETAIL__HAL_BATTERY_MSG__FUNCTIONS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdlib.h>

#include "rosidl_runtime_c/visibility_control.h"
#include "hal/msg/rosidl_generator_c__visibility_control.h"

#include "hal/msg/detail/hal_battery_msg__struct.h"

/// Initialize msg/HalBatteryMsg message.
/**
 * If the init function is called twice for the same message without
 * calling fini inbetween previously allocated memory will be leaked.
 * \param[in,out] msg The previously allocated message pointer.
 * Fields without a default value will not be initialized by this function.
 * You might want to call memset(msg, 0, sizeof(
 * hal__msg__HalBatteryMsg
 * )) before or use
 * hal__msg__HalBatteryMsg__create()
 * to allocate and initialize the message.
 * \return true if initialization was successful, otherwise false
 */
ROSIDL_GENERATOR_C_PUBLIC_hal
bool
hal__msg__HalBatteryMsg__init(hal__msg__HalBatteryMsg * msg);

/// Finalize msg/HalBatteryMsg message.
/**
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_hal
void
hal__msg__HalBatteryMsg__fini(hal__msg__HalBatteryMsg * msg);

/// Create msg/HalBatteryMsg message.
/**
 * It allocates the memory for the message, sets the memory to zero, and
 * calls
 * hal__msg__HalBatteryMsg__init().
 * \return The pointer to the initialized message if successful,
 * otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_hal
hal__msg__HalBatteryMsg *
hal__msg__HalBatteryMsg__create();

/// Destroy msg/HalBatteryMsg message.
/**
 * It calls
 * hal__msg__HalBatteryMsg__fini()
 * and frees the memory of the message.
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_hal
void
hal__msg__HalBatteryMsg__destroy(hal__msg__HalBatteryMsg * msg);

/// Check for msg/HalBatteryMsg message equality.
/**
 * \param[in] lhs The message on the left hand size of the equality operator.
 * \param[in] rhs The message on the right hand size of the equality operator.
 * \return true if messages are equal, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_hal
bool
hal__msg__HalBatteryMsg__are_equal(const hal__msg__HalBatteryMsg * lhs, const hal__msg__HalBatteryMsg * rhs);

/// Copy a msg/HalBatteryMsg message.
/**
 * This functions performs a deep copy, as opposed to the shallow copy that
 * plain assignment yields.
 *
 * \param[in] input The source message pointer.
 * \param[out] output The target message pointer, which must
 *   have been initialized before calling this function.
 * \return true if successful, or false if either pointer is null
 *   or memory allocation fails.
 */
ROSIDL_GENERATOR_C_PUBLIC_hal
bool
hal__msg__HalBatteryMsg__copy(
  const hal__msg__HalBatteryMsg * input,
  hal__msg__HalBatteryMsg * output);

/// Initialize array of msg/HalBatteryMsg messages.
/**
 * It allocates the memory for the number of elements and calls
 * hal__msg__HalBatteryMsg__init()
 * for each element of the array.
 * \param[in,out] array The allocated array pointer.
 * \param[in] size The size / capacity of the array.
 * \return true if initialization was successful, otherwise false
 * If the array pointer is valid and the size is zero it is guaranteed
 # to return true.
 */
ROSIDL_GENERATOR_C_PUBLIC_hal
bool
hal__msg__HalBatteryMsg__Sequence__init(hal__msg__HalBatteryMsg__Sequence * array, size_t size);

/// Finalize array of msg/HalBatteryMsg messages.
/**
 * It calls
 * hal__msg__HalBatteryMsg__fini()
 * for each element of the array and frees the memory for the number of
 * elements.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_hal
void
hal__msg__HalBatteryMsg__Sequence__fini(hal__msg__HalBatteryMsg__Sequence * array);

/// Create array of msg/HalBatteryMsg messages.
/**
 * It allocates the memory for the array and calls
 * hal__msg__HalBatteryMsg__Sequence__init().
 * \param[in] size The size / capacity of the array.
 * \return The pointer to the initialized array if successful, otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_hal
hal__msg__HalBatteryMsg__Sequence *
hal__msg__HalBatteryMsg__Sequence__create(size_t size);

/// Destroy array of msg/HalBatteryMsg messages.
/**
 * It calls
 * hal__msg__HalBatteryMsg__Sequence__fini()
 * on the array,
 * and frees the memory of the array.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_hal
void
hal__msg__HalBatteryMsg__Sequence__destroy(hal__msg__HalBatteryMsg__Sequence * array);

/// Check for msg/HalBatteryMsg message array equality.
/**
 * \param[in] lhs The message array on the left hand size of the equality operator.
 * \param[in] rhs The message array on the right hand size of the equality operator.
 * \return true if message arrays are equal in size and content, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_hal
bool
hal__msg__HalBatteryMsg__Sequence__are_equal(const hal__msg__HalBatteryMsg__Sequence * lhs, const hal__msg__HalBatteryMsg__Sequence * rhs);

/// Copy an array of msg/HalBatteryMsg messages.
/**
 * This functions performs a deep copy, as opposed to the shallow copy that
 * plain assignment yields.
 *
 * \param[in] input The source array pointer.
 * \param[out] output The target array pointer, which must
 *   have been initialized before calling this function.
 * \return true if successful, or false if either pointer
 *   is null or memory allocation fails.
 */
ROSIDL_GENERATOR_C_PUBLIC_hal
bool
hal__msg__HalBatteryMsg__Sequence__copy(
  const hal__msg__HalBatteryMsg__Sequence * input,
  hal__msg__HalBatteryMsg__Sequence * output);

#ifdef __cplusplus
}
#endif

#endif  // HAL__MSG__DETAIL__HAL_BATTERY_MSG__FUNCTIONS_H_
