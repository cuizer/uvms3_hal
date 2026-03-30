# generated from rosidl_generator_py/resource/_idl.py.em
# with input from hal:msg/HalAuxithrusterMsg.idl
# generated code does not contain a copyright notice


# Import statements for member types

import builtins  # noqa: E402, I100

# Member 'rpm'
# Member 'current'
# Member 'voltage'
# Member 'temp'
# Member 'esc_status'
# Member 'fault_status'
import numpy  # noqa: E402, I100

import rosidl_parser.definition  # noqa: E402, I100


class Metaclass_HalAuxithrusterMsg(type):
    """Metaclass of message 'HalAuxithrusterMsg'."""

    _CREATE_ROS_MESSAGE = None
    _CONVERT_FROM_PY = None
    _CONVERT_TO_PY = None
    _DESTROY_ROS_MESSAGE = None
    _TYPE_SUPPORT = None

    __constants = {
    }

    @classmethod
    def __import_type_support__(cls):
        try:
            from rosidl_generator_py import import_type_support
            module = import_type_support('hal')
        except ImportError:
            import logging
            import traceback
            logger = logging.getLogger(
                'hal.msg.HalAuxithrusterMsg')
            logger.debug(
                'Failed to import needed modules for type support:\n' +
                traceback.format_exc())
        else:
            cls._CREATE_ROS_MESSAGE = module.create_ros_message_msg__msg__hal_auxithruster_msg
            cls._CONVERT_FROM_PY = module.convert_from_py_msg__msg__hal_auxithruster_msg
            cls._CONVERT_TO_PY = module.convert_to_py_msg__msg__hal_auxithruster_msg
            cls._TYPE_SUPPORT = module.type_support_msg__msg__hal_auxithruster_msg
            cls._DESTROY_ROS_MESSAGE = module.destroy_ros_message_msg__msg__hal_auxithruster_msg

    @classmethod
    def __prepare__(cls, name, bases, **kwargs):
        # list constant names here so that they appear in the help text of
        # the message class under "Data and other attributes defined here:"
        # as well as populate each message instance
        return {
        }


class HalAuxithrusterMsg(metaclass=Metaclass_HalAuxithrusterMsg):
    """Message class 'HalAuxithrusterMsg'."""

    __slots__ = [
        '_rpm',
        '_current',
        '_voltage',
        '_temp',
        '_esc_status',
        '_fault_status',
    ]

    _fields_and_field_types = {
        'rpm': 'int16[6]',
        'current': 'int16[6]',
        'voltage': 'int16[6]',
        'temp': 'uint16[6]',
        'esc_status': 'uint8[6]',
        'fault_status': 'uint8[6]',
    }

    SLOT_TYPES = (
        rosidl_parser.definition.Array(rosidl_parser.definition.BasicType('int16'), 6),  # noqa: E501
        rosidl_parser.definition.Array(rosidl_parser.definition.BasicType('int16'), 6),  # noqa: E501
        rosidl_parser.definition.Array(rosidl_parser.definition.BasicType('int16'), 6),  # noqa: E501
        rosidl_parser.definition.Array(rosidl_parser.definition.BasicType('uint16'), 6),  # noqa: E501
        rosidl_parser.definition.Array(rosidl_parser.definition.BasicType('uint8'), 6),  # noqa: E501
        rosidl_parser.definition.Array(rosidl_parser.definition.BasicType('uint8'), 6),  # noqa: E501
    )

    def __init__(self, **kwargs):
        assert all('_' + key in self.__slots__ for key in kwargs.keys()), \
            'Invalid arguments passed to constructor: %s' % \
            ', '.join(sorted(k for k in kwargs.keys() if '_' + k not in self.__slots__))
        if 'rpm' not in kwargs:
            self.rpm = numpy.zeros(6, dtype=numpy.int16)
        else:
            self.rpm = kwargs.get('rpm')
        if 'current' not in kwargs:
            self.current = numpy.zeros(6, dtype=numpy.int16)
        else:
            self.current = kwargs.get('current')
        if 'voltage' not in kwargs:
            self.voltage = numpy.zeros(6, dtype=numpy.int16)
        else:
            self.voltage = kwargs.get('voltage')
        if 'temp' not in kwargs:
            self.temp = numpy.zeros(6, dtype=numpy.uint16)
        else:
            self.temp = kwargs.get('temp')
        if 'esc_status' not in kwargs:
            self.esc_status = numpy.zeros(6, dtype=numpy.uint8)
        else:
            self.esc_status = kwargs.get('esc_status')
        if 'fault_status' not in kwargs:
            self.fault_status = numpy.zeros(6, dtype=numpy.uint8)
        else:
            self.fault_status = kwargs.get('fault_status')

    def __repr__(self):
        typename = self.__class__.__module__.split('.')
        typename.pop()
        typename.append(self.__class__.__name__)
        args = []
        for s, t in zip(self.__slots__, self.SLOT_TYPES):
            field = getattr(self, s)
            fieldstr = repr(field)
            # We use Python array type for fields that can be directly stored
            # in them, and "normal" sequences for everything else.  If it is
            # a type that we store in an array, strip off the 'array' portion.
            if (
                isinstance(t, rosidl_parser.definition.AbstractSequence) and
                isinstance(t.value_type, rosidl_parser.definition.BasicType) and
                t.value_type.typename in ['float', 'double', 'int8', 'uint8', 'int16', 'uint16', 'int32', 'uint32', 'int64', 'uint64']
            ):
                if len(field) == 0:
                    fieldstr = '[]'
                else:
                    assert fieldstr.startswith('array(')
                    prefix = "array('X', "
                    suffix = ')'
                    fieldstr = fieldstr[len(prefix):-len(suffix)]
            args.append(s[1:] + '=' + fieldstr)
        return '%s(%s)' % ('.'.join(typename), ', '.join(args))

    def __eq__(self, other):
        if not isinstance(other, self.__class__):
            return False
        if any(self.rpm != other.rpm):
            return False
        if any(self.current != other.current):
            return False
        if any(self.voltage != other.voltage):
            return False
        if any(self.temp != other.temp):
            return False
        if any(self.esc_status != other.esc_status):
            return False
        if any(self.fault_status != other.fault_status):
            return False
        return True

    @classmethod
    def get_fields_and_field_types(cls):
        from copy import copy
        return copy(cls._fields_and_field_types)

    @builtins.property
    def rpm(self):
        """Message field 'rpm'."""
        return self._rpm

    @rpm.setter
    def rpm(self, value):
        if isinstance(value, numpy.ndarray):
            assert value.dtype == numpy.int16, \
                "The 'rpm' numpy.ndarray() must have the dtype of 'numpy.int16'"
            assert value.size == 6, \
                "The 'rpm' numpy.ndarray() must have a size of 6"
            self._rpm = value
            return
        if __debug__:
            from collections.abc import Sequence
            from collections.abc import Set
            from collections import UserList
            from collections import UserString
            assert \
                ((isinstance(value, Sequence) or
                  isinstance(value, Set) or
                  isinstance(value, UserList)) and
                 not isinstance(value, str) and
                 not isinstance(value, UserString) and
                 len(value) == 6 and
                 all(isinstance(v, int) for v in value) and
                 all(val >= -32768 and val < 32768 for val in value)), \
                "The 'rpm' field must be a set or sequence with length 6 and each value of type 'int' and each integer in [-32768, 32767]"
        self._rpm = numpy.array(value, dtype=numpy.int16)

    @builtins.property
    def current(self):
        """Message field 'current'."""
        return self._current

    @current.setter
    def current(self, value):
        if isinstance(value, numpy.ndarray):
            assert value.dtype == numpy.int16, \
                "The 'current' numpy.ndarray() must have the dtype of 'numpy.int16'"
            assert value.size == 6, \
                "The 'current' numpy.ndarray() must have a size of 6"
            self._current = value
            return
        if __debug__:
            from collections.abc import Sequence
            from collections.abc import Set
            from collections import UserList
            from collections import UserString
            assert \
                ((isinstance(value, Sequence) or
                  isinstance(value, Set) or
                  isinstance(value, UserList)) and
                 not isinstance(value, str) and
                 not isinstance(value, UserString) and
                 len(value) == 6 and
                 all(isinstance(v, int) for v in value) and
                 all(val >= -32768 and val < 32768 for val in value)), \
                "The 'current' field must be a set or sequence with length 6 and each value of type 'int' and each integer in [-32768, 32767]"
        self._current = numpy.array(value, dtype=numpy.int16)

    @builtins.property
    def voltage(self):
        """Message field 'voltage'."""
        return self._voltage

    @voltage.setter
    def voltage(self, value):
        if isinstance(value, numpy.ndarray):
            assert value.dtype == numpy.int16, \
                "The 'voltage' numpy.ndarray() must have the dtype of 'numpy.int16'"
            assert value.size == 6, \
                "The 'voltage' numpy.ndarray() must have a size of 6"
            self._voltage = value
            return
        if __debug__:
            from collections.abc import Sequence
            from collections.abc import Set
            from collections import UserList
            from collections import UserString
            assert \
                ((isinstance(value, Sequence) or
                  isinstance(value, Set) or
                  isinstance(value, UserList)) and
                 not isinstance(value, str) and
                 not isinstance(value, UserString) and
                 len(value) == 6 and
                 all(isinstance(v, int) for v in value) and
                 all(val >= -32768 and val < 32768 for val in value)), \
                "The 'voltage' field must be a set or sequence with length 6 and each value of type 'int' and each integer in [-32768, 32767]"
        self._voltage = numpy.array(value, dtype=numpy.int16)

    @builtins.property
    def temp(self):
        """Message field 'temp'."""
        return self._temp

    @temp.setter
    def temp(self, value):
        if isinstance(value, numpy.ndarray):
            assert value.dtype == numpy.uint16, \
                "The 'temp' numpy.ndarray() must have the dtype of 'numpy.uint16'"
            assert value.size == 6, \
                "The 'temp' numpy.ndarray() must have a size of 6"
            self._temp = value
            return
        if __debug__:
            from collections.abc import Sequence
            from collections.abc import Set
            from collections import UserList
            from collections import UserString
            assert \
                ((isinstance(value, Sequence) or
                  isinstance(value, Set) or
                  isinstance(value, UserList)) and
                 not isinstance(value, str) and
                 not isinstance(value, UserString) and
                 len(value) == 6 and
                 all(isinstance(v, int) for v in value) and
                 all(val >= 0 and val < 65536 for val in value)), \
                "The 'temp' field must be a set or sequence with length 6 and each value of type 'int' and each unsigned integer in [0, 65535]"
        self._temp = numpy.array(value, dtype=numpy.uint16)

    @builtins.property
    def esc_status(self):
        """Message field 'esc_status'."""
        return self._esc_status

    @esc_status.setter
    def esc_status(self, value):
        if isinstance(value, numpy.ndarray):
            assert value.dtype == numpy.uint8, \
                "The 'esc_status' numpy.ndarray() must have the dtype of 'numpy.uint8'"
            assert value.size == 6, \
                "The 'esc_status' numpy.ndarray() must have a size of 6"
            self._esc_status = value
            return
        if __debug__:
            from collections.abc import Sequence
            from collections.abc import Set
            from collections import UserList
            from collections import UserString
            assert \
                ((isinstance(value, Sequence) or
                  isinstance(value, Set) or
                  isinstance(value, UserList)) and
                 not isinstance(value, str) and
                 not isinstance(value, UserString) and
                 len(value) == 6 and
                 all(isinstance(v, int) for v in value) and
                 all(val >= 0 and val < 256 for val in value)), \
                "The 'esc_status' field must be a set or sequence with length 6 and each value of type 'int' and each unsigned integer in [0, 255]"
        self._esc_status = numpy.array(value, dtype=numpy.uint8)

    @builtins.property
    def fault_status(self):
        """Message field 'fault_status'."""
        return self._fault_status

    @fault_status.setter
    def fault_status(self, value):
        if isinstance(value, numpy.ndarray):
            assert value.dtype == numpy.uint8, \
                "The 'fault_status' numpy.ndarray() must have the dtype of 'numpy.uint8'"
            assert value.size == 6, \
                "The 'fault_status' numpy.ndarray() must have a size of 6"
            self._fault_status = value
            return
        if __debug__:
            from collections.abc import Sequence
            from collections.abc import Set
            from collections import UserList
            from collections import UserString
            assert \
                ((isinstance(value, Sequence) or
                  isinstance(value, Set) or
                  isinstance(value, UserList)) and
                 not isinstance(value, str) and
                 not isinstance(value, UserString) and
                 len(value) == 6 and
                 all(isinstance(v, int) for v in value) and
                 all(val >= 0 and val < 256 for val in value)), \
                "The 'fault_status' field must be a set or sequence with length 6 and each value of type 'int' and each unsigned integer in [0, 255]"
        self._fault_status = numpy.array(value, dtype=numpy.uint8)
