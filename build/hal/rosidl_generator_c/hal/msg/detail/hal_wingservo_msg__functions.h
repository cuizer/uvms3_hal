// generated from rosidl_generator_c/resource/idl__functions.h.em
// with input from hal:msg/HalWingservoMsg.idl
// generated code does not contain a copyright notice

#ifndef HAL__MSG__DETAIL__HAL_WINGSERVO_MSG__FUNCTIONS_H_
#define HAL__MSG__DETAIL__HAL_WINGSERVO_MSG__FUNCTIONS_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdbool.h>
#include <stdlib.h>

#include "rosidl_runtime_c/visibility_control.h"
#include "hal/msg/rosidl_generator_c__visibility_control.h"

#include "hal/msg/detail/hal_wingservo_msg__struct.h"

/// Initialize msg/HalWingservoMsg message.
/**
 * If the init function is called twice for the same message without
 * calling fini inbetween previously allocated memory will be leaked.
 * \param[in,out] msg The previously allocated message pointer.
 * Fields without a default value will not be initialized by this function.
 * You might want to call memset(msg, 0, sizeof(
 * hal__msg__HalWingservoMsg
 * )) before or use
 * hal__msg__HalWingservoMsg__create()
 * to allocate and initialize the message.
 * \return true if initialization was successful, otherwise false
 */
ROSIDL_GENERATOR_C_PUBLIC_hal
bool
hal__msg__HalWingservoMsg__init(hal__msg__HalWingservoMsg * msg);

/// Finalize msg/HalWingservoMsg message.
/**
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_hal
void
hal__msg__HalWingservoMsg__fini(hal__msg__HalWingservoMsg * msg);

/// Create msg/HalWingservoMsg message.
/**
 * It allocates the memory for the message, sets the memory to zero, and
 * calls
 * hal__msg__HalWingservoMsg__init().
 * \return The pointer to the initialized message if successful,
 * otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_hal
hal__msg__HalWingservoMsg *
hal__msg__HalWingservoMsg__create();

/// Destroy msg/HalWingservoMsg message.
/**
 * It calls
 * hal__msg__HalWingservoMsg__fini()
 * and frees the memory of the message.
 * \param[in,out] msg The allocated message pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_hal
void
hal__msg__HalWingservoMsg__destroy(hal__msg__HalWingservoMsg * msg);

/// Check for msg/HalWingservoMsg message equality.
/**
 * \param[in] lhs The message on the left hand size of the equality operator.
 * \param[in] rhs The message on the right hand size of the equality operator.
 * \return true if messages are equal, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_hal
bool
hal__msg__HalWingservoMsg__are_equal(const hal__msg__HalWingservoMsg * lhs, const hal__msg__HalWingservoMsg * rhs);

/// Copy a msg/HalWingservoMsg message.
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
hal__msg__HalWingservoMsg__copy(
  const hal__msg__HalWingservoMsg * input,
  hal__msg__HalWingservoMsg * output);

/// Initialize array of msg/HalWingservoMsg messages.
/**
 * It allocates the memory for the number of elements and calls
 * hal__msg__HalWingservoMsg__init()
 * for each element of the array.
 * \param[in,out] array The allocated array pointer.
 * \param[in] size The size / capacity of the array.
 * \return true if initialization was successful, otherwise false
 * If the array pointer is valid and the size is zero it is guaranteed
 # to return true.
 */
ROSIDL_GENERATOR_C_PUBLIC_hal
bool
hal__msg__HalWingservoMsg__Sequence__init(hal__msg__HalWingservoMsg__Sequence * array, size_t size);

/// Finalize array of msg/HalWingservoMsg messages.
/**
 * It calls
 * hal__msg__HalWingservoMsg__fini()
 * for each element of the array and frees the memory for the number of
 * elements.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_hal
void
hal__msg__HalWingservoMsg__Sequence__fini(hal__msg__HalWingservoMsg__Sequence * array);

/// Create array of msg/HalWingservoMsg messages.
/**
 * It allocates the memory for the array and calls
 * hal__msg__HalWingservoMsg__Sequence__init().
 * \param[in] size The size / capacity of the array.
 * \return The pointer to the initialized array if successful, otherwise NULL
 */
ROSIDL_GENERATOR_C_PUBLIC_hal
hal__msg__HalWingservoMsg__Sequence *
hal__msg__HalWingservoMsg__Sequence__create(size_t size);

/// Destroy array of msg/HalWingservoMsg messages.
/**
 * It calls
 * hal__msg__HalWingservoMsg__Sequence__fini()
 * on the array,
 * and frees the memory of the array.
 * \param[in,out] array The initialized array pointer.
 */
ROSIDL_GENERATOR_C_PUBLIC_hal
void
hal__msg__HalWingservoMsg__Sequence__destroy(hal__msg__HalWingservoMsg__Sequence * array);

/// Check for msg/HalWingservoMsg message array equality.
/**
 * \param[in] lhs The message array on the left hand size of the equality operator.
 * \param[in] rhs The message array on the right hand size of the equality operator.
 * \return true if message arrays are equal in size and content, otherwise false.
 */
ROSIDL_GENERATOR_C_PUBLIC_hal
bool
hal__msg__HalWingservoMsg__Sequence__are_equal(const hal__msg__HalWingservoMsg__Sequence * lhs, const hal__msg__HalWingservoMsg__Sequence * rhs);

/// Copy an array of msg/HalWingservoMsg messages.
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
hal__msg__HalWingservoMsg__Sequence__copy(
  const hal__msg__HalWingservoMsg__Sequence * input,
  hal__msg__HalWingservoMsg__Sequence * output);

#ifdef __cplusplus
}
#endif

#endif  // HAL__MSG__DETAIL__HAL_WINGSERVO_MSG__FUNCTIONS_H_
