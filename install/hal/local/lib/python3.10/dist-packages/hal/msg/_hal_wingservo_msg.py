# generated from rosidl_generator_py/resource/_idl.py.em
# with input from hal:msg/HalWingservoMsg.idl
# generated code does not contain a copyright notice


# Import statements for member types

import builtins  # noqa: E402, I100

# Member 'voltage'
# Member 'current'
# Member 'power'
# Member 'temperature'
# Member 'status'
import numpy  # noqa: E402, I100

import rosidl_parser.definition  # noqa: E402, I100


class Metaclass_HalWingservoMsg(type):
    """Metaclass of message 'HalWingservoMsg'."""

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
                'hal.msg.HalWingservoMsg')
            logger.debug(
                'Failed to import needed modules for type support:\n' +
                traceback.format_exc())
        else:
            cls._CREATE_ROS_MESSAGE = module.create_ros_message_msg__msg__hal_wingservo_msg
            cls._CONVERT_FROM_PY = module.convert_from_py_msg__msg__hal_wingservo_msg
            cls._CONVERT_TO_PY = module.convert_to_py_msg__msg__hal_wingservo_msg
            cls._TYPE_SUPPORT = module.type_support_msg__msg__hal_wingservo_msg
            cls._DESTROY_ROS_MESSAGE = module.destroy_ros_message_msg__msg__hal_wingservo_msg

    @classmethod
    def __prepare__(cls, name, bases, **kwargs):
        # list constant names here so that they appear in the help text of
        # the message class under "Data and other attributes defined here:"
        # as well as populate each message instance
        return {
        }


class HalWingservoMsg(metaclass=Metaclass_HalWingservoMsg):
    """Message class 'HalWingservoMsg'."""

    __slots__ = [
        '_voltage',
        '_current',
        '_power',
        '_temperature',
        '_status',
    ]

    _fields_and_field_types = {
        'voltage': 'int16[2]',
        'current': 'int16[2]',
        'power': 'uint16[2]',
        'temperature': 'uint16[2]',
        'status': 'uint8[2]',
    }

    SLOT_TYPES = (
        rosidl_parser.definition.Array(rosidl_parser.definition.BasicType('int16'), 2),  # noqa: E501
        rosidl_parser.definition.Array(rosidl_parser.definition.BasicType('int16'), 2),  # noqa: E501
        rosidl_parser.definition.Array(rosidl_parser.definition.BasicType('uint16'), 2),  # noqa: E501
        rosidl_parser.definition.Array(rosidl_parser.definition.BasicType('uint16'), 2),  # noqa: E501
        rosidl_parser.definition.Array(rosidl_parser.definition.BasicType('uint8'), 2),  # noqa: E501
    )

    def __init__(self, **kwargs):
        assert all('_' + key in self.__slots__ for key in kwargs.keys()), \
            'Invalid arguments passed to constructor: %s' % \
            ', '.join(sorted(k for k in kwargs.keys() if '_' + k not in self.__slots__))
        if 'voltage' not in kwargs:
            self.voltage = numpy.zeros(2, dtype=numpy.int16)
        else:
            self.voltage = kwargs.get('voltage')
        if 'current' not in kwargs:
            self.current = numpy.zeros(2, dtype=numpy.int16)
        else:
            self.current = kwargs.get('current')
        if 'power' not in kwargs:
            self.power = numpy.zeros(2, dtype=numpy.uint16)
        else:
            self.power = kwargs.get('power')
        if 'temperature' not in kwargs:
            self.temperature = numpy.zeros(2, dtype=numpy.uint16)
        else:
            self.temperature = kwargs.get('temperature')
        if 'status' not in kwargs:
            self.status = numpy.zeros(2, dtype=numpy.uint8)
        else:
            self.status = kwargs.get('status')

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
        if any(self.voltage != other.voltage):
            return False
        if any(self.current != other.current):
            return False
        if any(self.power != other.power):
            return False
        if any(self.temperature != other.temperature):
            return False
        if any(self.status != other.status):
            return False
        return True

    @classmethod
    def get_fields_and_field_types(cls):
        from copy import copy
        return copy(cls._fields_and_field_types)

    @builtins.property
    def voltage(self):
        """Message field 'voltage'."""
        return self._voltage

    @voltage.setter
    def voltage(self, value):
        if isinstance(value, numpy.ndarray):
            assert value.dtype == numpy.int16, \
                "The 'voltage' numpy.ndarray() must have the dtype of 'numpy.int16'"
            assert value.size == 2, \
                "The 'voltage' numpy.ndarray() must have a size of 2"
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
                 len(value) == 2 and
                 all(isinstance(v, int) for v in value) and
                 all(val >= -32768 and val < 32768 for val in value)), \
                "The 'voltage' field must be a set or sequence with length 2 and each value of type 'int' and each integer in [-32768, 32767]"
        self._voltage = numpy.array(value, dtype=numpy.int16)

    @builtins.property
    def current(self):
        """Message field 'current'."""
        return self._current

    @current.setter
    def current(self, value):
        if isinstance(value, numpy.ndarray):
            assert value.dtype == numpy.int16, \
                "The 'current' numpy.ndarray() must have the dtype of 'numpy.int16'"
            assert value.size == 2, \
                "The 'current' numpy.ndarray() must have a size of 2"
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
                 len(value) == 2 and
                 all(isinstance(v, int) for v in value) and
                 all(val >= -32768 and val < 32768 for val in value)), \
                "The 'current' field must be a set or sequence with length 2 and each value of type 'int' and each integer in [-32768, 32767]"
        self._current = numpy.array(value, dtype=numpy.int16)

    @builtins.property
    def power(self):
        """Message field 'power'."""
        return self._power

    @power.setter
    def power(self, value):
        if isinstance(value, numpy.ndarray):
            assert value.dtype == numpy.uint16, \
                "The 'power' numpy.ndarray() must have the dtype of 'numpy.uint16'"
            assert value.size == 2, \
                "The 'power' numpy.ndarray() must have a size of 2"
            self._power = value
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
                 len(value) == 2 and
                 all(isinstance(v, int) for v in value) and
                 all(val >= 0 and val < 65536 for val in value)), \
                "The 'power' field must be a set or sequence with length 2 and each value of type 'int' and each unsigned integer in [0, 65535]"
        self._power = numpy.array(value, dtype=numpy.uint16)

    @builtins.property
    def temperature(self):
        """Message field 'temperature'."""
        return self._temperature

    @temperature.setter
    def temperature(self, value):
        if isinstance(value, numpy.ndarray):
            assert value.dtype == numpy.uint16, \
                "The 'temperature' numpy.ndarray() must have the dtype of 'numpy.uint16'"
            assert value.size == 2, \
                "The 'temperature' numpy.ndarray() must have a size of 2"
            self._temperature = value
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
                 len(value) == 2 and
                 all(isinstance(v, int) for v in value) and
                 all(val >= 0 and val < 65536 for val in value)), \
                "The 'temperature' field must be a set or sequence with length 2 and each value of type 'int' and each unsigned integer in [0, 65535]"
        self._temperature = numpy.array(value, dtype=numpy.uint16)

    @builtins.property
    def status(self):
        """Message field 'status'."""
        return self._status

    @status.setter
    def status(self, value):
        if isinstance(value, numpy.ndarray):
            assert value.dtype == numpy.uint8, \
                "The 'status' numpy.ndarray() must have the dtype of 'numpy.uint8'"
            assert value.size == 2, \
                "The 'status' numpy.ndarray() must have a size of 2"
            self._status = value
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
                 len(value) == 2 and
                 all(isinstance(v, int) for v in value) and
                 all(val >= 0 and val < 256 for val in value)), \
                "The 'status' field must be a set or sequence with length 2 and each value of type 'int' and each unsigned integer in [0, 255]"
        self._status = numpy.array(value, dtype=numpy.uint8)
