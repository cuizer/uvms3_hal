#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};


#[link(name = "hal__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__hal__msg__HalBatteryMsg() -> *const std::ffi::c_void;
}

#[link(name = "hal__rosidl_generator_c")]
extern "C" {
    fn hal__msg__HalBatteryMsg__init(msg: *mut HalBatteryMsg) -> bool;
    fn hal__msg__HalBatteryMsg__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<HalBatteryMsg>, size: usize) -> bool;
    fn hal__msg__HalBatteryMsg__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<HalBatteryMsg>);
    fn hal__msg__HalBatteryMsg__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<HalBatteryMsg>, out_seq: *mut rosidl_runtime_rs::Sequence<HalBatteryMsg>) -> bool;
}

// Corresponds to hal__msg__HalBatteryMsg
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]

/// 电池状态消息
/// 节点消息命名: hal_battery_msg
/// 消息命名: uvms_battery_data

#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct HalBatteryMsg {
    /// 电池状态
    pub battery_status: u8,

    /// 电池电流 (单位: 0.1A)
    pub battery_current: i16,

    /// 循环次数
    pub cycle_count: u16,

    /// 剩余电量 (单位: 0.1AH)
    pub remain_capacity: u16,

    /// 总电量 (单位: 0.1AH)
    pub total_capacity: u16,

    /// 开关状态: 0=关闭, 1=打开
    pub switch_state: u8,

}



impl Default for HalBatteryMsg {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !hal__msg__HalBatteryMsg__init(&mut msg as *mut _) {
        panic!("Call to hal__msg__HalBatteryMsg__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for HalBatteryMsg {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { hal__msg__HalBatteryMsg__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { hal__msg__HalBatteryMsg__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { hal__msg__HalBatteryMsg__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for HalBatteryMsg {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for HalBatteryMsg where Self: Sized {
  const TYPE_NAME: &'static str = "hal/msg/HalBatteryMsg";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__hal__msg__HalBatteryMsg() }
  }
}


