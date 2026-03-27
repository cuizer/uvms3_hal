#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};




// Corresponds to hal__srv__HalBatteryControlSrv_Request

// This struct is not documented.
#[allow(missing_docs)]

#[allow(non_camel_case_types)]
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
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
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::srv::rmw::HalBatteryControlSrv_Request::default())
  }
}

impl rosidl_runtime_rs::Message for HalBatteryControlSrv_Request {
  type RmwMsg = super::srv::rmw::HalBatteryControlSrv_Request;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        command: msg.command,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      command: msg.command,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      command: msg.command,
    }
  }
}


// Corresponds to hal__srv__HalBatteryControlSrv_Response

// This struct is not documented.
#[allow(missing_docs)]

#[allow(non_camel_case_types)]
#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
#[derive(Clone, Debug, PartialEq, PartialOrd)]
pub struct HalBatteryControlSrv_Response {

    // This member is not documented.
    #[allow(missing_docs)]
    pub success: bool,


    // This member is not documented.
    #[allow(missing_docs)]
    pub message: std::string::String,

}



impl Default for HalBatteryControlSrv_Response {
  fn default() -> Self {
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::srv::rmw::HalBatteryControlSrv_Response::default())
  }
}

impl rosidl_runtime_rs::Message for HalBatteryControlSrv_Response {
  type RmwMsg = super::srv::rmw::HalBatteryControlSrv_Response;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        success: msg.success,
        message: msg.message.as_str().into(),
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      success: msg.success,
        message: msg.message.as_str().into(),
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      success: msg.success,
      message: msg.message.to_string(),
    }
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


