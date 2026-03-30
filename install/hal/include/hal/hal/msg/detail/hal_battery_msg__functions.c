// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from hal:msg/HalBatteryMsg.idl
// generated code does not contain a copyright notice
#include "hal/msg/detail/hal_battery_msg__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


bool
hal__msg__HalBatteryMsg__init(hal__msg__HalBatteryMsg * msg)
{
  if (!msg) {
    return false;
  }
  // battery_status
  // battery_current
  // cycle_count
  // remain_capacity
  // total_capacity
  // switch_state
  return true;
}

void
hal__msg__HalBatteryMsg__fini(hal__msg__HalBatteryMsg * msg)
{
  if (!msg) {
    return;
  }
  // battery_status
  // battery_current
  // cycle_count
  // remain_capacity
  // total_capacity
  // switch_state
}

bool
hal__msg__HalBatteryMsg__are_equal(const hal__msg__HalBatteryMsg * lhs, const hal__msg__HalBatteryMsg * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // battery_status
  if (lhs->battery_status != rhs->battery_status) {
    return false;
  }
  // battery_current
  if (lhs->battery_current != rhs->battery_current) {
    return false;
  }
  // cycle_count
  if (lhs->cycle_count != rhs->cycle_count) {
    return false;
  }
  // remain_capacity
  if (lhs->remain_capacity != rhs->remain_capacity) {
    return false;
  }
  // total_capacity
  if (lhs->total_capacity != rhs->total_capacity) {
    return false;
  }
  // switch_state
  if (lhs->switch_state != rhs->switch_state) {
    return false;
  }
  return true;
}

bool
hal__msg__HalBatteryMsg__copy(
  const hal__msg__HalBatteryMsg * input,
  hal__msg__HalBatteryMsg * output)
{
  if (!input || !output) {
    return false;
  }
  // battery_status
  output->battery_status = input->battery_status;
  // battery_current
  output->battery_current = input->battery_current;
  // cycle_count
  output->cycle_count = input->cycle_count;
  // remain_capacity
  output->remain_capacity = input->remain_capacity;
  // total_capacity
  output->total_capacity = input->total_capacity;
  // switch_state
  output->switch_state = input->switch_state;
  return true;
}

hal__msg__HalBatteryMsg *
hal__msg__HalBatteryMsg__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  hal__msg__HalBatteryMsg * msg = (hal__msg__HalBatteryMsg *)allocator.allocate(sizeof(hal__msg__HalBatteryMsg), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(hal__msg__HalBatteryMsg));
  bool success = hal__msg__HalBatteryMsg__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
hal__msg__HalBatteryMsg__destroy(hal__msg__HalBatteryMsg * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    hal__msg__HalBatteryMsg__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
hal__msg__HalBatteryMsg__Sequence__init(hal__msg__HalBatteryMsg__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  hal__msg__HalBatteryMsg * data = NULL;

  if (size) {
    data = (hal__msg__HalBatteryMsg *)allocator.zero_allocate(size, sizeof(hal__msg__HalBatteryMsg), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = hal__msg__HalBatteryMsg__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        hal__msg__HalBatteryMsg__fini(&data[i - 1]);
      }
      allocator.deallocate(data, allocator.state);
      return false;
    }
  }
  array->data = data;
  array->size = size;
  array->capacity = size;
  return true;
}

void
hal__msg__HalBatteryMsg__Sequence__fini(hal__msg__HalBatteryMsg__Sequence * array)
{
  if (!array) {
    return;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();

  if (array->data) {
    // ensure that data and capacity values are consistent
    assert(array->capacity > 0);
    // finalize all array elements
    for (size_t i = 0; i < array->capacity; ++i) {
      hal__msg__HalBatteryMsg__fini(&array->data[i]);
    }
    allocator.deallocate(array->data, allocator.state);
    array->data = NULL;
    array->size = 0;
    array->capacity = 0;
  } else {
    // ensure that data, size, and capacity values are consistent
    assert(0 == array->size);
    assert(0 == array->capacity);
  }
}

hal__msg__HalBatteryMsg__Sequence *
hal__msg__HalBatteryMsg__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  hal__msg__HalBatteryMsg__Sequence * array = (hal__msg__HalBatteryMsg__Sequence *)allocator.allocate(sizeof(hal__msg__HalBatteryMsg__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = hal__msg__HalBatteryMsg__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
hal__msg__HalBatteryMsg__Sequence__destroy(hal__msg__HalBatteryMsg__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    hal__msg__HalBatteryMsg__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
hal__msg__HalBatteryMsg__Sequence__are_equal(const hal__msg__HalBatteryMsg__Sequence * lhs, const hal__msg__HalBatteryMsg__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!hal__msg__HalBatteryMsg__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
hal__msg__HalBatteryMsg__Sequence__copy(
  const hal__msg__HalBatteryMsg__Sequence * input,
  hal__msg__HalBatteryMsg__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(hal__msg__HalBatteryMsg);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    hal__msg__HalBatteryMsg * data =
      (hal__msg__HalBatteryMsg *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!hal__msg__HalBatteryMsg__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          hal__msg__HalBatteryMsg__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!hal__msg__HalBatteryMsg__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
