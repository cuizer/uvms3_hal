# generated from rosidl_generator_py/resource/_idl.py.em
# with input from hal:msg/HalBatteryMsg.idl
# generated code does not contain a copyright notice


# Import statements for member types

import builtins  # noqa: E402, I100

import rosidl_parser.definition  # noqa: E402, I100


class Metaclass_HalBatteryMsg(type):
    """Metaclass of message 'HalBatteryMsg'."""

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
                'hal.msg.HalBatteryMsg')
            logger.debug(
                'Failed to import needed modules for type support:\n' +
                traceback.format_exc())
        else:
            cls._CREATE_ROS_MESSAGE = module.create_ros_message_msg__msg__hal_battery_msg
            cls._CONVERT_FROM_PY = module.convert_from_py_msg__msg__hal_battery_msg
            cls._CONVERT_TO_PY = module.convert_to_py_msg__msg__hal_battery_msg
            cls._TYPE_SUPPORT = module.type_support_msg__msg__hal_battery_msg
            cls._DESTROY_ROS_MESSAGE = module.destroy_ros_message_msg__msg__hal_battery_msg

    @classmethod
    def __prepare__(cls, name, bases, **kwargs):
        # list constant names here so that they appear in the help text of
        # the message class under "Data and other attributes defined here:"
        # as well as populate each message instance
        return {
        }


class HalBatteryMsg(metaclass=Metaclass_HalBatteryMsg):
    """Message class 'HalBatteryMsg'."""

    __slots__ = [
        '_battery_status',
        '_battery_current',
        '_cycle_count',
        '_remain_capacity',
        '_total_capacity',
        '_switch_state',
    ]

    _fields_and_field_types = {
        'battery_status': 'octet',
        'battery_current': 'int16',
        'cycle_count': 'uint16',
        'remain_capacity': 'uint16',
        'total_capacity': 'uint16',
        'switch_state': 'octet',
    }

    SLOT_TYPES = (
        rosidl_parser.definition.BasicType('octet'),  # noqa: E501
        rosidl_parser.definition.BasicType('int16'),  # noqa: E501
        rosidl_parser.definition.BasicType('uint16'),  # noqa: E501
        rosidl_parser.definition.BasicType('uint16'),  # noqa: E501
        rosidl_parser.definition.BasicType('uint16'),  # noqa: E501
        rosidl_parser.definition.BasicType('octet'),  # noqa: E501
    )

    def __init__(self, **kwargs):
        assert all('_' + key in self.__slots__ for key in kwargs.keys()), \
            'Invalid arguments passed to constructor: %s' % \
            ', '.join(sorted(k for k in kwargs.keys() if '_' + k not in self.__slots__))
        self.battery_status = kwargs.get('battery_status', bytes([0]))
        self.battery_current = kwargs.get('battery_current', int())
        self.cycle_count = kwargs.get('cycle_count', int())
        self.remain_capacity = kwargs.get('remain_capacity', int())
        self.total_capacity = kwargs.get('total_capacity', int())
        self.switch_state = kwargs.get('switch_state', bytes([0]))

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
        if self.battery_status != other.battery_status:
            return False
        if self.battery_current != other.battery_current:
            return False
        if self.cycle_count != other.cycle_count:
            return False
        if self.remain_capacity != other.remain_capacity:
            return False
        if self.total_capacity != other.total_capacity:
            return False
        if self.switch_state != other.switch_state:
            return False
        return True

    @classmethod
    def get_fields_and_field_types(cls):
        from copy import copy
        return copy(cls._fields_and_field_types)

    @builtins.property
    def battery_status(self):
        """Message field 'battery_status'."""
        return self._battery_status

    @battery_status.setter
    def battery_status(self, value):
        if __debug__:
            from collections.abc import ByteString
            assert \
                (isinstance(value, (bytes, ByteString)) and
                 len(value) == 1), \
                "The 'battery_status' field must be of type 'bytes' or 'ByteString' with length 1"
        self._battery_status = value

    @builtins.property
    def battery_current(self):
        """Message field 'battery_current'."""
        return self._battery_current

    @battery_current.setter
    def battery_current(self, value):
        if __debug__:
            assert \
                isinstance(value, int), \
                "The 'battery_current' field must be of type 'int'"
            assert value >= -32768 and value < 32768, \
                "The 'battery_current' field must be an integer in [-32768, 32767]"
        self._battery_current = value

    @builtins.property
    def cycle_count(self):
        """Message field 'cycle_count'."""
        return self._cycle_count

    @cycle_count.setter
    def cycle_count(self, value):
        if __debug__:
            assert \
                isinstance(value, int), \
                "The 'cycle_count' field must be of type 'int'"
            assert value >= 0 and value < 65536, \
                "The 'cycle_count' field must be an unsigned integer in [0, 65535]"
        self._cycle_count = value

    @builtins.property
    def remain_capacity(self):
        """Message field 'remain_capacity'."""
        return self._remain_capacity

    @remain_capacity.setter
    def remain_capacity(self, value):
        if __debug__:
            assert \
                isinstance(value, int), \
                "The 'remain_capacity' field must be of type 'int'"
            assert value >= 0 and value < 65536, \
                "The 'remain_capacity' field must be an unsigned integer in [0, 65535]"
        self._remain_capacity = value

    @builtins.property
    def total_capacity(self):
        """Message field 'total_capacity'."""
        return self._total_capacity

    @total_capacity.setter
    def total_capacity(self, value):
        if __debug__:
            assert \
                isinstance(value, int), \
                "The 'total_capacity' field must be of type 'int'"
            assert value >= 0 and value < 65536, \
                "The 'total_capacity' field must be an unsigned integer in [0, 65535]"
        self._total_capacity = value

    @builtins.property
    def switch_state(self):
        """Message field 'switch_state'."""
        return self._switch_state

    @switch_state.setter
    def switch_state(self, value):
        if __debug__:
            from collections.abc import ByteString
            assert \
                (isinstance(value, (bytes, ByteString)) and
                 len(value) == 1), \
                "The 'switch_state' field must be of type 'bytes' or 'ByteString' with length 1"
        self._switch_state = value
