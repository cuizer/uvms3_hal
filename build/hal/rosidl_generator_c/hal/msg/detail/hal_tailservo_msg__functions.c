// generated from rosidl_generator_c/resource/idl__functions.c.em
// with input from hal:msg/HalTailservoMsg.idl
// generated code does not contain a copyright notice
#include "hal/msg/detail/hal_tailservo_msg__functions.h"

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "rcutils/allocator.h"


bool
hal__msg__HalTailservoMsg__init(hal__msg__HalTailservoMsg * msg)
{
  if (!msg) {
    return false;
  }
  // voltage
  // current
  // power
  // temperature
  // status
  return true;
}

void
hal__msg__HalTailservoMsg__fini(hal__msg__HalTailservoMsg * msg)
{
  if (!msg) {
    return;
  }
  // voltage
  // current
  // power
  // temperature
  // status
}

bool
hal__msg__HalTailservoMsg__are_equal(const hal__msg__HalTailservoMsg * lhs, const hal__msg__HalTailservoMsg * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  // voltage
  for (size_t i = 0; i < 4; ++i) {
    if (lhs->voltage[i] != rhs->voltage[i]) {
      return false;
    }
  }
  // current
  for (size_t i = 0; i < 4; ++i) {
    if (lhs->current[i] != rhs->current[i]) {
      return false;
    }
  }
  // power
  for (size_t i = 0; i < 4; ++i) {
    if (lhs->power[i] != rhs->power[i]) {
      return false;
    }
  }
  // temperature
  for (size_t i = 0; i < 4; ++i) {
    if (lhs->temperature[i] != rhs->temperature[i]) {
      return false;
    }
  }
  // status
  for (size_t i = 0; i < 4; ++i) {
    if (lhs->status[i] != rhs->status[i]) {
      return false;
    }
  }
  return true;
}

bool
hal__msg__HalTailservoMsg__copy(
  const hal__msg__HalTailservoMsg * input,
  hal__msg__HalTailservoMsg * output)
{
  if (!input || !output) {
    return false;
  }
  // voltage
  for (size_t i = 0; i < 4; ++i) {
    output->voltage[i] = input->voltage[i];
  }
  // current
  for (size_t i = 0; i < 4; ++i) {
    output->current[i] = input->current[i];
  }
  // power
  for (size_t i = 0; i < 4; ++i) {
    output->power[i] = input->power[i];
  }
  // temperature
  for (size_t i = 0; i < 4; ++i) {
    output->temperature[i] = input->temperature[i];
  }
  // status
  for (size_t i = 0; i < 4; ++i) {
    output->status[i] = input->status[i];
  }
  return true;
}

hal__msg__HalTailservoMsg *
hal__msg__HalTailservoMsg__create()
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  hal__msg__HalTailservoMsg * msg = (hal__msg__HalTailservoMsg *)allocator.allocate(sizeof(hal__msg__HalTailservoMsg), allocator.state);
  if (!msg) {
    return NULL;
  }
  memset(msg, 0, sizeof(hal__msg__HalTailservoMsg));
  bool success = hal__msg__HalTailservoMsg__init(msg);
  if (!success) {
    allocator.deallocate(msg, allocator.state);
    return NULL;
  }
  return msg;
}

void
hal__msg__HalTailservoMsg__destroy(hal__msg__HalTailservoMsg * msg)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (msg) {
    hal__msg__HalTailservoMsg__fini(msg);
  }
  allocator.deallocate(msg, allocator.state);
}


bool
hal__msg__HalTailservoMsg__Sequence__init(hal__msg__HalTailservoMsg__Sequence * array, size_t size)
{
  if (!array) {
    return false;
  }
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  hal__msg__HalTailservoMsg * data = NULL;

  if (size) {
    data = (hal__msg__HalTailservoMsg *)allocator.zero_allocate(size, sizeof(hal__msg__HalTailservoMsg), allocator.state);
    if (!data) {
      return false;
    }
    // initialize all array elements
    size_t i;
    for (i = 0; i < size; ++i) {
      bool success = hal__msg__HalTailservoMsg__init(&data[i]);
      if (!success) {
        break;
      }
    }
    if (i < size) {
      // if initialization failed finalize the already initialized array elements
      for (; i > 0; --i) {
        hal__msg__HalTailservoMsg__fini(&data[i - 1]);
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
hal__msg__HalTailservoMsg__Sequence__fini(hal__msg__HalTailservoMsg__Sequence * array)
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
      hal__msg__HalTailservoMsg__fini(&array->data[i]);
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

hal__msg__HalTailservoMsg__Sequence *
hal__msg__HalTailservoMsg__Sequence__create(size_t size)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  hal__msg__HalTailservoMsg__Sequence * array = (hal__msg__HalTailservoMsg__Sequence *)allocator.allocate(sizeof(hal__msg__HalTailservoMsg__Sequence), allocator.state);
  if (!array) {
    return NULL;
  }
  bool success = hal__msg__HalTailservoMsg__Sequence__init(array, size);
  if (!success) {
    allocator.deallocate(array, allocator.state);
    return NULL;
  }
  return array;
}

void
hal__msg__HalTailservoMsg__Sequence__destroy(hal__msg__HalTailservoMsg__Sequence * array)
{
  rcutils_allocator_t allocator = rcutils_get_default_allocator();
  if (array) {
    hal__msg__HalTailservoMsg__Sequence__fini(array);
  }
  allocator.deallocate(array, allocator.state);
}

bool
hal__msg__HalTailservoMsg__Sequence__are_equal(const hal__msg__HalTailservoMsg__Sequence * lhs, const hal__msg__HalTailservoMsg__Sequence * rhs)
{
  if (!lhs || !rhs) {
    return false;
  }
  if (lhs->size != rhs->size) {
    return false;
  }
  for (size_t i = 0; i < lhs->size; ++i) {
    if (!hal__msg__HalTailservoMsg__are_equal(&(lhs->data[i]), &(rhs->data[i]))) {
      return false;
    }
  }
  return true;
}

bool
hal__msg__HalTailservoMsg__Sequence__copy(
  const hal__msg__HalTailservoMsg__Sequence * input,
  hal__msg__HalTailservoMsg__Sequence * output)
{
  if (!input || !output) {
    return false;
  }
  if (output->capacity < input->size) {
    const size_t allocation_size =
      input->size * sizeof(hal__msg__HalTailservoMsg);
    rcutils_allocator_t allocator = rcutils_get_default_allocator();
    hal__msg__HalTailservoMsg * data =
      (hal__msg__HalTailservoMsg *)allocator.reallocate(
      output->data, allocation_size, allocator.state);
    if (!data) {
      return false;
    }
    // If reallocation succeeded, memory may or may not have been moved
    // to fulfill the allocation request, invalidating output->data.
    output->data = data;
    for (size_t i = output->capacity; i < input->size; ++i) {
      if (!hal__msg__HalTailservoMsg__init(&output->data[i])) {
        // If initialization of any new item fails, roll back
        // all previously initialized items. Existing items
        // in output are to be left unmodified.
        for (; i-- > output->capacity; ) {
          hal__msg__HalTailservoMsg__fini(&output->data[i]);
        }
        return false;
      }
    }
    output->capacity = input->size;
  }
  output->size = input->size;
  for (size_t i = 0; i < input->size; ++i) {
    if (!hal__msg__HalTailservoMsg__copy(
        &(input->data[i]), &(output->data[i])))
    {
      return false;
    }
  }
  return true;
}
