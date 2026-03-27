#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};



#[link(name = "hal__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__hal__srv__HalBatteryControlSrv_Request() -> *const std::ffi::c_void;
}

#[link(name = "hal__rosidl_generator_c")]
extern "C" {
    fn hal__srv__HalBatteryControlSrv_Request__init(msg: *mut HalBatteryControlSrv_Request) -> bool;
    fn hal__srv__HalBatteryControlSrv_Request__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<HalBatteryControlSrv_Request>, size: usize) -> bool;
    fn hal__srv__HalBatteryControlSrv_Request__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<HalBatteryControlSrv_Request>);
    fn hal__srv__HalBatteryControlSrv_Request__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<HalBatteryControlSrv_Request>, out_seq: *mut rosidl_runtime_rs::Sequence<HalBatteryControlSrv_Request>) -> bool;
}

// Corresponds to hal__srv__HalBatteryControlSrv_Request
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]


// This struct is not documented.
#[allow(missing_docs)]

#[allow(non_camel_case_types)]
#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct HalBatteryControlSrv_Request {
    /// 请求部分
    /// 控制命令：
    /// 01 = 12V开
    /// 02 = 12V关
    /// 03 = 24V开
    /// 04 = 24V关
    /// 05 = 72V开
    /// 06 = 72V关
    pub command: u8,

}



impl Default for HalBatteryControlSrv_Request {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !hal__srv__HalBatteryControlSrv_Request__init(&mut msg as *mut _) {
        panic!("Call to hal__srv__HalBatteryControlSrv_Request__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for HalBatteryControlSrv_Request {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { hal__srv__HalBatteryControlSrv_Request__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { hal__srv__HalBatteryControlSrv_Request__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { hal__srv__HalBatteryControlSrv_Request__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for HalBatteryControlSrv_Request {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for HalBatteryControlSrv_Request where Self: Sized {
  const TYPE_NAME: &'static str = "hal/srv/HalBatteryControlSrv_Request";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__hal__srv__HalBatteryControlSrv_Request() }
  }
}


#[link(name = "hal__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_message_type_support_handle__hal__srv__HalBatteryControlSrv_Response() -> *const std::ffi::c_void;
}

#[link(name = "hal__rosidl_generator_c")]
extern "C" {
    fn hal__srv__HalBatteryControlSrv_Response__init(msg: *mut HalBatteryControlSrv_Response) -> bool;
    fn hal__srv__HalBatteryControlSrv_Response__Sequence__init(seq: *mut rosidl_runtime_rs::Sequence<HalBatteryControlSrv_Response>, size: usize) -> bool;
    fn hal__srv__HalBatteryControlSrv_Response__Sequence__fini(seq: *mut rosidl_runtime_rs::Sequence<HalBatteryControlSrv_Response>);
    fn hal__srv__HalBatteryControlSrv_Response__Sequence__copy(in_seq: &rosidl_runtime_rs::Sequence<HalBatteryControlSrv_Response>, out_seq: *mut rosidl_runtime_rs::Sequence<HalBatteryControlSrv_Response>) -> bool;
}

// Corresponds to hal__srv__HalBatteryControlSrv_Response
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]


// This struct is not documented.
#[allow(missing_docs)]

#[allow(non_camel_case_types)]
#[repr(C)]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct HalBatteryControlSrv_Response {

    // This member is not documented.
    #[allow(missing_docs)]
    pub success: bool,


    // This member is not documented.
    #[allow(missing_docs)]
    pub message: rosidl_runtime_rs::String,

}



impl Default for HalBatteryControlSrv_Response {
  fn default() -> Self {
    unsafe {
      let mut msg = std::mem::zeroed();
      if !hal__srv__HalBatteryControlSrv_Response__init(&mut msg as *mut _) {
        panic!("Call to hal__srv__HalBatteryControlSrv_Response__init() failed");
      }
      msg
    }
  }
}

impl rosidl_runtime_rs::SequenceAlloc for HalBatteryControlSrv_Response {
  fn sequence_init(seq: &mut rosidl_runtime_rs::Sequence<Self>, size: usize) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { hal__srv__HalBatteryControlSrv_Response__Sequence__init(seq as *mut _, size) }
  }
  fn sequence_fini(seq: &mut rosidl_runtime_rs::Sequence<Self>) {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { hal__srv__HalBatteryControlSrv_Response__Sequence__fini(seq as *mut _) }
  }
  fn sequence_copy(in_seq: &rosidl_runtime_rs::Sequence<Self>, out_seq: &mut rosidl_runtime_rs::Sequence<Self>) -> bool {
    // SAFETY: This is safe since the pointer is guaranteed to be valid/initialized.
    unsafe { hal__srv__HalBatteryControlSrv_Response__Sequence__copy(in_seq, out_seq as *mut _) }
  }
}

impl rosidl_runtime_rs::Message for HalBatteryControlSrv_Response {
  type RmwMsg = Self;
  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> { msg_cow }
  fn from_rmw_message(msg: Self::RmwMsg) -> Self { msg }
}

impl rosidl_runtime_rs::RmwMessage for HalBatteryControlSrv_Response where Self: Sized {
  const TYPE_NAME: &'static str = "hal/srv/HalBatteryControlSrv_Response";
  fn get_type_support() -> *const std::ffi::c_void {
    // SAFETY: No preconditions for this function.
    unsafe { rosidl_typesupport_c__get_message_type_support_handle__hal__srv__HalBatteryControlSrv_Response() }
  }
}






#[link(name = "hal__rosidl_typesupport_c")]
extern "C" {
    fn rosidl_typesupport_c__get_service_type_support_handle__hal__srv__HalBatteryControlSrv() -> *const std::ffi::c_void;
}

// Corresponds to hal__srv__HalBatteryControlSrv
#[allow(missing_docs, non_camel_case_types)]
pub struct HalBatteryControlSrv;

impl rosidl_runtime_rs::Service for HalBatteryControlSrv {
    type Request = HalBatteryControlSrv_Request;
    type Response = HalBatteryControlSrv_Response;

    fn get_type_support() -> *const std::ffi::c_void {
        // SAFETY: No preconditions for this function.
        unsafe { rosidl_typesupport_c__get_service_type_support_handle__hal__srv__HalBatteryControlSrv() }
    }
}


