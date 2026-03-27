#[cfg(feature = "serde")]
use serde::{Deserialize, Serialize};



// Corresponds to hal__msg__HalBatteryMsg
/// 电池状态消息
/// 节点消息命名: hal_battery_msg
/// 消息命名: uvms_battery_data

#[cfg_attr(feature = "serde", derive(Deserialize, Serialize))]
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
    <Self as rosidl_runtime_rs::Message>::from_rmw_message(super::msg::rmw::HalBatteryMsg::default())
  }
}

impl rosidl_runtime_rs::Message for HalBatteryMsg {
  type RmwMsg = super::msg::rmw::HalBatteryMsg;

  fn into_rmw_message(msg_cow: std::borrow::Cow<'_, Self>) -> std::borrow::Cow<'_, Self::RmwMsg> {
    match msg_cow {
      std::borrow::Cow::Owned(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
        battery_status: msg.battery_status,
        battery_current: msg.battery_current,
        cycle_count: msg.cycle_count,
        remain_capacity: msg.remain_capacity,
        total_capacity: msg.total_capacity,
        switch_state: msg.switch_state,
      }),
      std::borrow::Cow::Borrowed(msg) => std::borrow::Cow::Owned(Self::RmwMsg {
      battery_status: msg.battery_status,
      battery_current: msg.battery_current,
      cycle_count: msg.cycle_count,
      remain_capacity: msg.remain_capacity,
      total_capacity: msg.total_capacity,
      switch_state: msg.switch_state,
      })
    }
  }

  fn from_rmw_message(msg: Self::RmwMsg) -> Self {
    Self {
      battery_status: msg.battery_status,
      battery_current: msg.battery_current,
      cycle_count: msg.cycle_count,
      remain_capacity: msg.remain_capacity,
      total_capacity: msg.total_capacity,
      switch_state: msg.switch_state,
    }
  }
}


