// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from hal:msg/HalMainthrusterMsg.idl
// generated code does not contain a copyright notice
#include "hal/msg/detail/hal_mainthruster_msg__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


bool
hal__msg__HalMainthrusterMsg__init(hal__msg__HalMainthrusterMsg * msg)
{
  if (!msg) {
    return false;
  }
  // rpm
  // current
  // voltage
  // fault_status
  return true;
}

void
hal__msg__HalMainthrusterMsg__fini(hal__msg__HalMainthrusterMsg * msg)
{
  if (!msg) {
    return;
  }
  // rpm
  // current
  // voltage
  // fault_status
}

bool
hal__msg__HalMainthrusterMsg__are_equal(const hal__msg__HalMainthrusterMsg * lhs, const hal__msg__HalMainthrusterMsg * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // rpm
  if (lhs->rpm != rhs->rpm) {
    return false;
  }
  // current
  if (lhs->current != rhs->current) {
    return false;
  }
  // voltage
  if (lhs->voltage != rhs->voltage) {
    return false;
  }
  // fault_status
  if (lhs->fault_status != rhs->fault_status) {
    return false;
  }
  return true;
}

bool
hal__msg__HalMainthrusterMsg__copy(
  const hal__msg__HalMainthrusterMsg * input,
  hal__msg__HalMainthrusterMsg * output)
{
  if (!input || !output) {
    return false;
  }
  // rpm
  output->rpm = input->rpm;
  // current
  output->current = input->current;
  // voltage
  output->voltage = input->voltage;
  // fault_status
  output->fault_status = input->fault_status;
  return true;
}

hal__msg__HalMainthrusterMsg *
hal__msg__HalMainthrusterMsg__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  hal__msg__HalMainthrusterMsg * msg = (hal__msg__HalMainthrusterMsg *)allocator.allocate(sizeof(hal__msg__HalMainthrusterMsg), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(hal__msg__HalMainthrusterMsg));
  bool success = hal__msg__HalMainthrusterMsg__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
hal__msg__HalMainthrusterMsg__destroy(hal__msg__HalMainthrusterMsg * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    hal__msg__HalMainthrusterMsg__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
hal__msg__HalMainthrusterMsg__Sequence__init(hal__msg__HalMainthrusterMsg__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  hal__msg__HalMainthrusterMsg * data = NULL;

  if (size) {
    data = (hal__msg__HalMainthrusterMsg *)allocator.zero_allocate(size, sizeof(hal__msg__HalMainthrusterMsg), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = hal__msg__HalMainthrusterMsg__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        hal__msg__HalMainthrusterMsg__fini(&data[i - 1]);
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
hal__msg__HalMainthrusterMsg__Sequence__fini(hal__msg__HalMainthrusterMsg__Sequence * array)
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
      hal__msg__HalMainthrusterMsg__fini(&array->data[i]);
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

hal__msg__HalMainthrusterMsg__Sequence *
hal__msg__HalMainthrusterMsg__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  hal__msg__HalMainthrusterMsg__Sequence * array = (hal__msg__HalMainthrusterMsg__Sequence *)allocator.allocate(sizeof(hal__msg__HalMainthrusterMsg__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = hal__msg__HalMainthrusterMsg__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
hal__msg__HalMainthrusterMsg__Sequence__destroy(hal__msg__HalMainthrusterMsg__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    hal__msg__HalMainthrusterMsg__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
hal__msg__HalMainthrusterMsg__Sequence__are_equal(const hal__msg__HalMainthrusterMsg__Sequence * lhs, const hal__msg__HalMainthrusterMsg__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!hal__msg__HalMainthrusterMsg__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
hal__msg__HalMainthrusterMsg__Sequence__copy(
  const hal__msg__HalMainthrusterMsg__Sequence * input,
  hal__msg__HalMainthrusterMsg__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(hal__msg__HalMainthrusterMsg);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    hal__msg__HalMainthrusterMsg * data =
      (hal__msg__HalMainthrusterMsg *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!hal__msg__HalMainthrusterMsg__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          hal__msg__HalMainthrusterMsg__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!hal__msg__HalMainthrusterMsg__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
