// generated from rosidl_generator_py/resource/_idl_support.c.em
// with input from hal:msg/HalBatteryMsg.idl
// generated code does not contain a copyright notice
#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION
#include <Python.h>
#include <stdbool.h>
#ifndef _WIN32
# pragma GCC diagnostic push
# pragma GCC diagnostic ignored "-Wunused-function"
#endif
#include "numpy/ndarrayobject.h"
#ifndef _WIN32
# pragma GCC diagnostic pop
#endif
#include "rosidl_runtime_c/visibility_control.h"
#include "hal/msg/detail/hal_battery_msg__struct.h"
#include "hal/msg/detail/hal_battery_msg__functions.h"


ROSIDL_GENERATOR_C_EXPORT
bool hal__msg__hal_battery_msg__convert_from_py(PyObject * _pymsg, void * _ros_message)
{
  // check that the passed message is of the expected Python class
  {
    char full_classname_dest[39];
    {
      char * class_name = NULL;
      char * module_name = NULL;
      {
        PyObject * class_attr = PyObject_GetAttrString(_pymsg, "__class__");
        if (class_attr) {
          PyObject * name_attr = PyObject_GetAttrString(class_attr, "__name__");
          if (name_attr) {
            class_name = (char *)PyUnicode_1BYTE_DATA(name_attr);
            Py_DECREF(name_attr);
          }
          PyObject * module_attr = PyObject_GetAttrString(class_attr, "__module__");
          if (module_attr) {
            module_name = (char *)PyUnicode_1BYTE_DATA(module_attr);
            Py_DECREF(module_attr);
          }
          Py_DECREF(class_attr);
        }
      }
      if (!class_name || !module_name) {
        return false;
      }
      snprintf(full_classname_dest, sizeof(full_classname_dest), "%s.%s", module_name, class_name);
    }
    assert(strncmp("hal.msg._hal_battery_msg.HalBatteryMsg", full_classname_dest, 38) == 0);
  }
  hal__msg__HalBatteryMsg * ros_message = _ros_message;
  {  // battery_status
    PyObject * field = PyObject_GetAttrString(_pymsg, "battery_status");
    if (!field) {
      return false;
    }
    assert(PyBytes_Check(field));
    ros_message->battery_status = PyBytes_AS_STRING(field)[0];
    Py_DECREF(field);
  }
  {  // battery_current
    PyObject * field = PyObject_GetAttrString(_pymsg, "battery_current");
    if (!field) {
      return false;
    }
    assert(PyLong_Check(field));
    ros_message->battery_current = (int16_t)PyLong_AsLong(field);
    Py_DECREF(field);
  }
  {  // cycle_count
    PyObject * field = PyObject_GetAttrString(_pymsg, "cycle_count");
    if (!field) {
      return false;
    }
    assert(PyLong_Check(field));
    ros_message->cycle_count = (uint16_t)PyLong_AsUnsignedLong(field);
    Py_DECREF(field);
  }
  {  // remain_capacity
    PyObject * field = PyObject_GetAttrString(_pymsg, "remain_capacity");
    if (!field) {
      return false;
    }
    assert(PyLong_Check(field));
    ros_message->remain_capacity = (uint16_t)PyLong_AsUnsignedLong(field);
    Py_DECREF(field);
  }
  {  // total_capacity
    PyObject * field = PyObject_GetAttrString(_pymsg, "total_capacity");
    if (!field) {
      return false;
    }
    assert(PyLong_Check(field));
    ros_message->total_capacity = (uint16_t)PyLong_AsUnsignedLong(field);
    Py_DECREF(field);
  }
  {  // switch_state
    PyObject * field = PyObject_GetAttrString(_pymsg, "switch_state");
    if (!field) {
      return false;
    }
    assert(PyBytes_Check(field));
    ros_message->switch_state = PyBytes_AS_STRING(field)[0];
    Py_DECREF(field);
  }

  return true;
}

ROSIDL_GENERATOR_C_EXPORT
PyObject * hal__msg__hal_battery_msg__convert_to_py(void * raw_ros_message)
{
  /* NOTE(esteve): Call constructor of HalBatteryMsg */
  PyObject * _pymessage = NULL;
  {
    PyObject * pymessage_module = PyImport_ImportModule("hal.msg._hal_battery_msg");
    assert(pymessage_module);
    PyObject * pymessage_class = PyObject_GetAttrString(pymessage_module, "HalBatteryMsg");
    assert(pymessage_class);
    Py_DECREF(pymessage_module);
    _pymessage = PyObject_CallObject(pymessage_class, NULL);
    Py_DECREF(pymessage_class);
    if (!_pymessage) {
      return NULL;
    }
  }
  hal__msg__HalBatteryMsg * ros_message = (hal__msg__HalBatteryMsg *)raw_ros_message;
  {  // battery_status
    PyObject * field = NULL;
    field = PyBytes_FromStringAndSize((const char *)&ros_message->battery_status, 1);
    if (!field) {
      return NULL;
    }
    {
      int rc = PyObject_SetAttrString(_pymessage, "battery_status", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // battery_current
    PyObject * field = NULL;
    field = PyLong_FromLong(ros_message->battery_current);
    {
      int rc = PyObject_SetAttrString(_pymessage, "battery_current", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // cycle_count
    PyObject * field = NULL;
    field = PyLong_FromUnsignedLong(ros_message->cycle_count);
    {
      int rc = PyObject_SetAttrString(_pymessage, "cycle_count", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // remain_capacity
    PyObject * field = NULL;
    field = PyLong_FromUnsignedLong(ros_message->remain_capacity);
    {
      int rc = PyObject_SetAttrString(_pymessage, "remain_capacity", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // total_capacity
    PyObject * field = NULL;
    field = PyLong_FromUnsignedLong(ros_message->total_capacity);
    {
      int rc = PyObject_SetAttrString(_pymessage, "total_capacity", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }
  {  // switch_state
    PyObject * field = NULL;
    field = PyBytes_FromStringAndSize((const char *)&ros_message->switch_state, 1);
    if (!field) {
      return NULL;
    }
    {
      int rc = PyObject_SetAttrString(_pymessage, "switch_state", field);
      Py_DECREF(field);
      if (rc) {
        return NULL;
      }
    }
  }

  // ownership of _pymessage is transferred to the caller
  return _pymessage;
}
