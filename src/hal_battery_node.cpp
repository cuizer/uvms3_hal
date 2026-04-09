#include <memory>
#include <string>
#include <cstring>  // 用于 memset
#include <map>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include <lifecycle_msgs/msg/state.hpp>
#include <lifecycle_msgs/msg/transition.hpp>
#include <stdexcept>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include "hal/msg/hal_battery.hpp"
#include "hal/srv/hal_battery_control_srv.hpp"
#include <chrono>

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_CYAN    "\033[36m"

using namespace std::chrono_literals;

namespace hal {

// BMS数据结构
struct BmsData {
  float total_voltage_48v;      
  float total_current_48v;      
  float remain_capacity_48v;    
  float full_capacity_48v;      
  float soc_48v;                
  uint16_t cycle_count_48v;     
  float max_cell_vol_48v;       
  uint8_t max_cell_id_48v;      
  float min_cell_vol_48v;       
  uint8_t min_cell_id_48v;      
  int8_t max_temp_48v;          
  int8_t min_temp_48v;          
  uint8_t cell_count_48v;       
  uint32_t protection_status_48v; 
  bool mos_charge_state_48v;    
  bool mos_discharge_state_48v; 
  int8_t temperatures_48v[8];   
  uint16_t cell_voltages_48v[32]; 
  
  float total_voltage_72v;      
  float total_current_72v;      
  float remain_capacity_72v;    
  float full_capacity_72v;      
  float soc_72v;                
  uint16_t cycle_count_72v;     
  float max_cell_vol_72v;       
  uint8_t max_cell_id_72v;      
  float min_cell_vol_72v;       
  uint8_t min_cell_id_72v;      
  int8_t max_temp_72v;          
  int8_t min_temp_72v;          
  uint8_t cell_count_72v;       
  uint32_t protection_status_72v; 
  bool mos_charge_state_72v;    
  bool mos_discharge_state_72v; 
  int8_t temperatures_72v[8];   
  uint16_t cell_voltages_72v[32]; 
  
  bool switch_state_12v;        
  bool switch_state_24v;        
  bool switch_state_48v;        
  bool switch_state_72v;        
  
  bool updated;                 
};

enum class BatteryType {
  BATTERY_12V = 0,
  BATTERY_24V = 1,
  BATTERY_72V = 2
};

struct BatteryStatusData {
  BmsData bms_data;         
};

class CanInterface {
public:
  CanInterface(const std::string & can_interface);
  ~CanInterface();
  
  bool init();
  void close();
  bool send_control_command(bool state_12v, bool state_24v, bool state_72v);
  bool receive_status(BatteryStatusData & status);
  
private:
  std::string can_interface_;
  int socket_fd_;
  bool set_nonblocking(bool enable);
};

CanInterface::CanInterface(const std::string & can_interface)
: can_interface_(can_interface), socket_fd_(-1) {}

CanInterface::~CanInterface() { close(); }

bool CanInterface::init()
{
  if (socket_fd_ >= 0) return true;

  socket_fd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (socket_fd_ < 0) return false;

  struct ifreq ifr;
  std::strncpy(ifr.ifr_name, can_interface_.c_str(), IFNAMSIZ - 1);
  if (ioctl(socket_fd_, SIOCGIFINDEX, &ifr) < 0) {
    close();
    return false;
  }

  struct sockaddr_can addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;

  if (bind(socket_fd_, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    close();
    return false;
  }

  set_nonblocking(true);
  return true;
}

void CanInterface::close()
{
  if (socket_fd_ >= 0) {
    ::close(socket_fd_);
    socket_fd_ = -1;
  }
}

bool CanInterface::set_nonblocking(bool enable)
{
  int flags = fcntl(socket_fd_, F_GETFL, 0);
  if (flags < 0) return false;
  if (enable) flags |= O_NONBLOCK;
  else flags &= ~O_NONBLOCK;
  return fcntl(socket_fd_, F_SETFL, flags) >= 0;
}

bool CanInterface::send_control_command(bool state_12v, bool state_24v, bool state_72v)
{
  if (socket_fd_ < 0) return false;

  struct can_frame frame;
  std::memset(&frame, 0, sizeof(frame));
  
  frame.can_id = 0x100;
  frame.can_dlc = 8;
  frame.data[0] = 0x0A;
  frame.data[1] = 0x09;
  frame.data[2] = state_12v ? 0x01 : 0x00; 
  frame.data[3] = state_24v ? 0x01 : 0x00; 
  frame.data[4] = state_72v ? 0x01 : 0x00; 
  frame.data[5] = 0; frame.data[6] = 0; frame.data[7] = 0;

  ssize_t nbytes = write(socket_fd_, &frame, sizeof(struct can_frame));
  return (nbytes == sizeof(struct can_frame));
}

bool CanInterface::receive_status(BatteryStatusData & status)
{
  if (socket_fd_ < 0) return false;

  struct can_frame frame;
  ssize_t nbytes = read(socket_fd_, &frame, sizeof(struct can_frame));
  
  if (nbytes < 0) return false;
  if (static_cast<size_t>(nbytes) < sizeof(struct can_frame)) return false;

  uint32_t can_id = frame.can_id;
  
  if (can_id >= 0x710 && can_id <= 0x716) {
    uint8_t group_idx = can_id - 0x710;
    uint8_t start_idx = group_idx * 4;
    if (start_idx + 3 < 32 && frame.can_dlc >= 8) {
      status.bms_data.cell_voltages_48v[start_idx] = (frame.data[0] << 8) | frame.data[1];
      status.bms_data.cell_voltages_48v[start_idx + 1] = (frame.data[2] << 8) | frame.data[3];
      status.bms_data.cell_voltages_48v[start_idx + 2] = (frame.data[4] << 8) | frame.data[5];
      status.bms_data.cell_voltages_48v[start_idx + 3] = (frame.data[6] << 8) | frame.data[7];
      status.bms_data.updated = true;
      return false;
    }
  }

  if (can_id >= 0x730 && can_id <= 0x736) {
    uint8_t group_idx = can_id - 0x730;
    uint8_t start_idx = group_idx * 4;
    if (start_idx + 3 < 32 && frame.can_dlc >= 8) {
      status.bms_data.cell_voltages_72v[start_idx] = (frame.data[0] << 8) | frame.data[1];
      status.bms_data.cell_voltages_72v[start_idx + 1] = (frame.data[2] << 8) | frame.data[3];
      status.bms_data.cell_voltages_72v[start_idx + 2] = (frame.data[4] << 8) | frame.data[5];
      status.bms_data.cell_voltages_72v[start_idx + 3] = (frame.data[6] << 8) | frame.data[7];
      status.bms_data.updated = true;
      return false;
    }
  }
  
  switch (can_id) {
    case 0x718: 
      if (frame.can_dlc >= 8) {
        for(int i = 0; i < 8; i++) status.bms_data.temperatures_48v[i] = (int8_t)frame.data[i];
        status.bms_data.updated = true;
        return false;
      }
      break;
    case 0x719:
      if (frame.can_dlc >= 8) {
        status.bms_data.total_voltage_48v = ((frame.data[0] << 8) | frame.data[1]) * 0.1f;
        status.bms_data.total_current_48v = ((int16_t)((frame.data[2] << 8) | frame.data[3])) * 0.1f;
        status.bms_data.remain_capacity_48v = ((frame.data[4] << 8) | frame.data[5]) * 0.1f;
        status.bms_data.full_capacity_48v = ((frame.data[6] << 8) | frame.data[7]) * 0.1f;
        status.bms_data.updated = true;
        return true;
      }
      break;
    case 0x71A:
      if (frame.can_dlc >= 8) {
        status.bms_data.soc_48v = ((frame.data[0] << 8) | frame.data[1]) * 0.1f;
        status.bms_data.cycle_count_48v = (frame.data[4] << 8) | frame.data[5];
        status.bms_data.updated = true;
        return false;
      }
      break;
    case 0x71B:
      if (frame.can_dlc >= 8) {
        status.bms_data.max_cell_vol_48v = ((frame.data[0] << 8) | frame.data[1]) * 0.001f;
        status.bms_data.max_cell_id_48v = frame.data[2];
        status.bms_data.min_cell_vol_48v = ((frame.data[3] << 8) | frame.data[4]) * 0.001f;
        status.bms_data.min_cell_id_48v = frame.data[5];
        status.bms_data.updated = true;
        return false;
      }
      break;
    case 0x71C:
      if (frame.can_dlc >= 8) {
        status.bms_data.max_temp_48v = (int8_t)frame.data[0];
        status.bms_data.min_temp_48v = (int8_t)frame.data[4];
        status.bms_data.updated = true;
        return false;
      }
      break;
    case 0x71E:
      if (frame.can_dlc >= 8) {
        status.bms_data.cell_count_48v = frame.data[3];
        status.bms_data.updated = true;
        return false;
      }
      break;
    case 0x723:
      if (frame.can_dlc >= 8) {
        status.bms_data.protection_status_48v = (frame.data[0] << 24) | (frame.data[1] << 16) | (frame.data[2] << 8) | frame.data[3];
        status.bms_data.mos_charge_state_48v = (frame.data[3] & 0x01) != 0;
        status.bms_data.mos_discharge_state_48v = (frame.data[3] & 0x02) != 0;
        status.bms_data.updated = true;
        return true;
      }
      break;
    case 0x738: 
      if (frame.can_dlc >= 8) {
        for(int i = 0; i < 8; i++) status.bms_data.temperatures_72v[i] = (int8_t)frame.data[i];
        status.bms_data.updated = true;
        return false;
      }
      break;
    case 0x739: 
      if (frame.can_dlc >= 8) {
        status.bms_data.total_voltage_72v = ((frame.data[0] << 8) | frame.data[1]) * 0.1f;
        status.bms_data.total_current_72v = ((int16_t)((frame.data[2] << 8) | frame.data[3])) * 0.1f;
        status.bms_data.remain_capacity_72v = ((frame.data[4] << 8) | frame.data[5]) * 0.1f;
        status.bms_data.full_capacity_72v = ((frame.data[6] << 8) | frame.data[7]) * 0.1f;
        status.bms_data.updated = true;
        return true;
      }
      break;
    case 0x73A: 
      if (frame.can_dlc >= 8) {
        status.bms_data.soc_72v = ((frame.data[0] << 8) | frame.data[1]) * 0.1f;
        status.bms_data.cycle_count_72v = (frame.data[4] << 8) | frame.data[5];
        status.bms_data.updated = true;
        return false;
      }
      break;
    case 0x743: 
      if (frame.can_dlc >= 8) {
        status.bms_data.protection_status_72v = (frame.data[0] << 24) | (frame.data[1] << 16) | (frame.data[2] << 8) | frame.data[3];
        status.bms_data.mos_charge_state_72v = (frame.data[3] & 0x01) != 0;
        status.bms_data.mos_discharge_state_72v = (frame.data[3] & 0x02) != 0;
        status.bms_data.updated = true;
        return true;
      }
      break;
    case 0x726: 
      if (frame.can_dlc >= 8) {
        status.bms_data.switch_state_12v = (frame.data[0] != 0);
        status.bms_data.switch_state_24v = (frame.data[1] != 0);
        status.bms_data.switch_state_72v = (frame.data[2] != 0); 
        status.bms_data.switch_state_48v = status.bms_data.switch_state_72v; 
        status.bms_data.updated = true;
        return true;
      }
      break;
    case 0x725:
    case 0x745: 
      status.bms_data.updated = true;
      return false;
    default:
      return false;
  }
  return false;
}

} // namespace hal

class PowerHalNode : public rclcpp_lifecycle::LifecycleNode
{
public:
  explicit PowerHalNode(const std::string & node_name)
  : rclcpp_lifecycle::LifecycleNode(node_name),
    can_interface_("can0"),
    simulation_mode_(false),
    safe_state_(false),
    last_comm_time_(this->now())
  {
    this->declare_parameter("simulation_mode", false);
    this->declare_parameter("voltage_threshold", 10.0);
    this->declare_parameter("current_threshold", 100.0);
    this->declare_parameter("temperature_threshold", 60.0);
    this->declare_parameter("communication_timeout", 5.0);
    
    simulation_mode_ = this->get_parameter("simulation_mode").as_bool();
    voltage_threshold_ = this->get_parameter("voltage_threshold").as_double();
    current_threshold_ = this->get_parameter("current_threshold").as_double();
    temperature_threshold_ = this->get_parameter("temperature_threshold").as_double();
    communication_timeout_ = this->get_parameter("communication_timeout").as_double();
    
    battery_states_[hal::BatteryType::BATTERY_12V] = false;
    battery_states_[hal::BatteryType::BATTERY_24V] = false;
    battery_states_[hal::BatteryType::BATTERY_72V] = false;

    // 清零全局的BMS状态池，解决乱码问题
    std::memset(&bms_data_, 0, sizeof(bms_data_));
  }

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_configure(const rclcpp_lifecycle::State &)
  {
    if (!simulation_mode_) {
      can_interface_.init();
    }
    
    battery_pub_ = this->create_publisher<hal::msg::HalBattery>("/hal/battery", 10);
    battery_control_srv_ = this->create_service<hal::srv::HalBatteryControlSrv>(
      "/hal/batterycontrol",
      std::bind(&PowerHalNode::on_battery_control_service, this, std::placeholders::_1, std::placeholders::_2));
    timer_ = this->create_wall_timer(100ms, std::bind(&PowerHalNode::on_timer, this));
    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
  }

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_activate(const rclcpp_lifecycle::State &)
  {
    battery_pub_->on_activate(); 
    timer_->reset();
    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
  }

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_deactivate(const rclcpp_lifecycle::State &)
  {
    battery_pub_->on_deactivate();
    timer_->cancel();
    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
  }

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_cleanup(const rclcpp_lifecycle::State &)
  {
    timer_.reset();
    battery_pub_.reset();
    battery_control_srv_.reset();
    can_interface_.close();
    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
  }

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_shutdown(const rclcpp_lifecycle::State &)
  {
    timer_.reset();
    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
  }

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_error(const rclcpp_lifecycle::State &)
  {
    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
  }

private:
void on_timer()
{
  if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) {
    return;
  }

  if (simulation_mode_) {
    publish_simulated_status();
  } else {
    // ==========================================
    // 【新增架构】：每次循环（100ms）强制向单片机下发一次当前的开关状态
    // 这种高频“心跳式”下发可以防止单片机漏读指令或意外复位
    // ==========================================
    can_interface_.send_control_command(
        battery_states_[hal::BatteryType::BATTERY_12V],
        battery_states_[hal::BatteryType::BATTERY_24V],
        battery_states_[hal::BatteryType::BATTERY_72V]
    );

    hal::BatteryStatusData status;
    status.bms_data = this->bms_data_; 
    
    while (can_interface_.receive_status(status)) {
      last_comm_time_ = this->now();
    }
    
    this->bms_data_ = status.bms_data;
    
    auto msg = create_battery_msg(status);
    battery_pub_->publish(msg);
    log_battery_status(msg);
  }
}
  
  hal::msg::HalBattery create_battery_msg(const hal::BatteryStatusData& status)
  {
    auto msg = hal::msg::HalBattery();
    
    msg.timestamp = this->now().nanoseconds() / 1000000;
    
    msg.battery_status_48v = status.bms_data.protection_status_48v != 0 ? 1 : 0;
    msg.battery_voltage_48v = static_cast<uint16_t>(status.bms_data.total_voltage_48v * 10);
    msg.battery_current_48v = static_cast<int16_t>(status.bms_data.total_current_48v * 10);
    msg.cycle_count_48v = status.bms_data.cycle_count_48v;
    msg.battery_temperature_48v = static_cast<uint16_t>(status.bms_data.max_temp_48v);
    msg.remain_capacity_48v = static_cast<uint16_t>(status.bms_data.remain_capacity_48v * 10);
    msg.total_capacity_48v = static_cast<uint16_t>(status.bms_data.full_capacity_48v * 10);
    
    msg.battery_status_72v = status.bms_data.protection_status_72v != 0 ? 1 : 0;
    msg.battery_voltage_72v = static_cast<uint16_t>(status.bms_data.total_voltage_72v * 10);
    msg.battery_current_72v = static_cast<int16_t>(status.bms_data.total_current_72v * 10);
    msg.cycle_count_72v = status.bms_data.cycle_count_72v;
    msg.battery_temperature_72v = static_cast<uint16_t>(status.bms_data.max_temp_72v);
    msg.remain_capacity_72v = static_cast<uint16_t>(status.bms_data.remain_capacity_72v * 10);
    msg.total_capacity_72v = static_cast<uint16_t>(status.bms_data.full_capacity_72v * 10);
    
    msg.switch_state_12v = status.bms_data.switch_state_12v ? 1 : 0;
    msg.switch_state_24v = status.bms_data.switch_state_24v ? 1 : 0;
    msg.switch_state_72v = status.bms_data.switch_state_72v ? 1 : 0;
    
    return msg;
  }
  
  void publish_simulated_status() { /* 模拟模式简化省略 */ }
  
  void log_battery_status(const hal::msg::HalBattery& msg)
  {
    RCLCPP_INFO(this->get_logger(), "%s电池状态信息:%s", COLOR_CYAN, COLOR_RESET);
    RCLCPP_INFO(this->get_logger(), "  时间戳: %ld ms", msg.timestamp);
    RCLCPP_INFO(this->get_logger(), "  48V: 状态=%s 电压=%.1fV 电流=%.1fA 循环=%d 温度=%d℃ 电量=%.1f/%.1fAH", 
        msg.battery_status_48v ? "异常" : "正常",
        msg.battery_voltage_48v / 10.0, 
        msg.battery_current_48v / 10.0, 
        msg.cycle_count_48v,
        msg.battery_temperature_48v, 
        msg.remain_capacity_48v / 10.0, 
        msg.total_capacity_48v / 10.0);
    RCLCPP_INFO(this->get_logger(), "  72V: 状态=%s 电压=%.1fV 电流=%.1fA 循环=%d 温度=%d℃ 电量=%.1f/%.1fAH", 
        msg.battery_status_72v ? "异常" : "正常",
        msg.battery_voltage_72v / 10.0, 
        msg.battery_current_72v / 10.0, 
        msg.cycle_count_72v,
        msg.battery_temperature_72v, 
        msg.remain_capacity_72v / 10.0, 
        msg.total_capacity_72v / 10.0);
        
    // 【新增】：在日志中清晰打印底层真实反馈上来的三个开关状态
    RCLCPP_INFO(this->get_logger(), "  开关反馈: 12V=[%s]  24V=[%s]  72V=[%s]", 
        msg.switch_state_12v ? "开启" : "关闭",
        msg.switch_state_24v ? "开启" : "关闭",
        msg.switch_state_72v ? "开启" : "关闭");
  }

  void on_battery_control_service(
    const std::shared_ptr<hal::srv::HalBatteryControlSrv::Request> request,
    std::shared_ptr<hal::srv::HalBatteryControlSrv::Response> response) 
  {
    RCLCPP_INFO(this->get_logger(), "收到电池控制服务请求: command=%d", request->command);
    
    bool success = false;
    std::string message = "";
    
    switch (request->command) {
      case 1: success = control_battery(hal::BatteryType::BATTERY_12V, true, "12V"); message = success ? "12V电池已开启" : "12V电池开启失败"; break;
      case 2: success = control_battery(hal::BatteryType::BATTERY_12V, false, "12V"); message = success ? "12V电池已关闭" : "12V电池关闭失败"; break;
      case 3: success = control_battery(hal::BatteryType::BATTERY_24V, true, "24V"); message = success ? "24V电池已开启" : "24V电池开启失败"; break;
      case 4: success = control_battery(hal::BatteryType::BATTERY_24V, false, "24V"); message = success ? "24V电池已关闭" : "24V电池关闭失败"; break;
      case 5: success = control_battery(hal::BatteryType::BATTERY_72V, true, "72V"); message = success ? "72V电池已开启" : "72V电池开启失败"; break;
      case 6: success = control_battery(hal::BatteryType::BATTERY_72V, false, "72V"); message = success ? "72V电池已关闭" : "72V电池关闭失败"; break;
      default: success = false; message = "无效的控制命令"; break;
    }
    
    response->success = success;
    response->message = message;
  }
  
  bool control_battery(hal::BatteryType battery_type, bool turn_on, const std::string& name)
  {
    // 现在只更新内部的“期望状态”，不再这里发 CAN 报文
    battery_states_[battery_type] = turn_on;
    RCLCPP_INFO(this->get_logger(), "已更新期望控制状态: 【%s】-> %s (将由定时器以10Hz持续下发)", name.c_str(), turn_on ? "开启" : "关闭");
    return true;
  }
  
  // ==========================================
  // 【核心修复】：在这里补上了缺失的变量声明
  // ==========================================
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp_lifecycle::LifecyclePublisher<hal::msg::HalBattery>::SharedPtr battery_pub_;
  rclcpp::Service<hal::srv::HalBatteryControlSrv>::SharedPtr battery_control_srv_;
  hal::CanInterface can_interface_;
  std::map<hal::BatteryType, bool> battery_states_;
  hal::BmsData bms_data_; // <--- 关键！
  bool simulation_mode_;
  bool safe_state_;
  rclcpp::Time last_comm_time_;
  double voltage_threshold_;
  double current_threshold_;
  double temperature_threshold_;
  double communication_timeout_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<PowerHalNode>("hal_battery_node");
  rclcpp::spin(node->get_node_base_interface());
  rclcpp::shutdown();
  return 0;
}