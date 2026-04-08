#include <memory>
#include <string>
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
  // 48V电池数据
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
  
  // 72V电池数据
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
  
  // 开关状态
  bool switch_state_12v;        
  bool switch_state_24v;        
  bool switch_state_48v;        
  bool switch_state_72v;        
  
  bool updated;                 
};

// 电池状态数据结构
enum class BatteryType {
  BATTERY_12V = 0,
  BATTERY_24V = 1,
  BATTERY_72V = 2
};

struct BatteryStatusData {
  BmsData bms_data;         
};

// CAN接口类
class CanInterface {
public:
  CanInterface(const std::string & can_interface);
  ~CanInterface();
  
  bool init();
  void close();
  // 修改发送函数，同时下发3个开关状态
  bool send_control_command(bool state_12v, bool state_24v, bool state_72v);
  bool receive_status(BatteryStatusData & status);
  
private:
  std::string can_interface_;
  int socket_fd_;
  
  bool set_nonblocking(bool enable);
};

CanInterface::CanInterface(const std::string & can_interface)
: can_interface_(can_interface), socket_fd_(-1)
{
}

CanInterface::~CanInterface()
{
  close();
}

bool CanInterface::init()
{
  if (socket_fd_ >= 0) {
    RCLCPP_WARN(rclcpp::get_logger("CanInterface"), "CAN interface already initialized");
    return true;
  }

  socket_fd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
  if (socket_fd_ < 0) {
    RCLCPP_ERROR(rclcpp::get_logger("CanInterface"), "Failed to create CAN socket");
    return false;
  }

  struct ifreq ifr;
  std::strncpy(ifr.ifr_name, can_interface_.c_str(), IFNAMSIZ - 1);
  if (ioctl(socket_fd_, SIOCGIFINDEX, &ifr) < 0) {
    RCLCPP_ERROR(rclcpp::get_logger("CanInterface"), "Failed to get CAN interface index");
    close();
    return false;
  }

  struct sockaddr_can addr;
  std::memset(&addr, 0, sizeof(addr));
  addr.can_family = AF_CAN;
  addr.can_ifindex = ifr.ifr_ifindex;

  if (bind(socket_fd_, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    RCLCPP_ERROR(rclcpp::get_logger("CanInterface"), "Failed to bind CAN socket");
    close();
    return false;
  }

  if (!set_nonblocking(true)) {
    RCLCPP_WARN(rclcpp::get_logger("CanInterface"), "Failed to set non-blocking mode");
  }

  RCLCPP_INFO(rclcpp::get_logger("CanInterface"), "CAN interface %s initialized successfully", can_interface_.c_str());
  return true;
}

void CanInterface::close()
{
  if (socket_fd_ >= 0) {
    ::close(socket_fd_);
    socket_fd_ = -1;
    RCLCPP_INFO(rclcpp::get_logger("CanInterface"), "CAN interface closed");
  }
}

bool CanInterface::set_nonblocking(bool enable)
{
  int flags = fcntl(socket_fd_, F_GETFL, 0);
  if (flags < 0) {
    return false;
  }
  
  if (enable) {
    flags |= O_NONBLOCK;
  } else {
    flags &= ~O_NONBLOCK;
  }
  
  return fcntl(socket_fd_, F_SETFL, flags) >= 0;
}

// 实现：将三个开关状态分别映射到 data[2], data[3], data[4]
bool CanInterface::send_control_command(bool state_12v, bool state_24v, bool state_72v)
{
  if (socket_fd_ < 0) {
    RCLCPP_ERROR(rclcpp::get_logger("CanInterface"), "CAN interface not initialized");
    return false;
  }

  struct can_frame frame;
  std::memset(&frame, 0, sizeof(frame));
  
  frame.can_id = 0x100;
  frame.can_dlc = 8;
  
  frame.data[0] = 0x0A;
  frame.data[1] = 0x09;
  frame.data[2] = state_12v ? 0x01 : 0x00; // 控制 12V (PE13)
  frame.data[3] = state_24v ? 0x01 : 0x00; // 控制 24V (PE9)
  frame.data[4] = state_72v ? 0x01 : 0x00; // 控制 72V (PA2)
  frame.data[5] = 0;
  frame.data[6] = 0;
  frame.data[7] = 0;

  ssize_t nbytes = write(socket_fd_, &frame, sizeof(struct can_frame));
  if (nbytes != sizeof(struct can_frame)) {
    RCLCPP_ERROR(rclcpp::get_logger("CanInterface"), "Failed to send CAN frame");
    return false;
  }

  return true;
}

bool CanInterface::receive_status(BatteryStatusData & status)
{
  if (socket_fd_ < 0) {
    return false;
  }

  struct can_frame frame;
  ssize_t nbytes = read(socket_fd_, &frame, sizeof(struct can_frame));
  
  if (nbytes < 0) {
    if (errno == EAGAIN || errno == EWOULDBLOCK) {
      return false;
    }
    RCLCPP_ERROR(rclcpp::get_logger("CanInterface"), "Failed to read CAN frame");
    return false;
  }

  if (static_cast<size_t>(nbytes) < sizeof(struct can_frame)) {
    return false;
  }

  uint32_t can_id = frame.can_id;
  
  // 处理48V电池单体电压数据 (CAN ID 0x710-0x716，释放0x718给温度)
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

  // 处理72V电池单体电压数据 (CAN ID 0x730-0x736)
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
    // ---- 48V 数据解析 ----
    case 0x718: // 48V 温度数据
      if (frame.can_dlc >= 8) {
        for(int i = 0; i < 8; i++) {
          status.bms_data.temperatures_48v[i] = (int8_t)frame.data[i];
        }
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
        status.bms_data.protection_status_48v = (frame.data[0] << 24) | (frame.data[1] << 16) | 
                                            (frame.data[2] << 8) | frame.data[3];
        status.bms_data.mos_charge_state_48v = (frame.data[3] & 0x01) != 0;
        status.bms_data.mos_discharge_state_48v = (frame.data[3] & 0x02) != 0;
        status.bms_data.updated = true;
        return true;
      }
      break;

    // ---- 72V 数据解析 (CAN ID 从 0x8xx 修正为 0x73x 频段) ----
    case 0x738: // 72V 温度数据
      if (frame.can_dlc >= 8) {
        for(int i = 0; i < 8; i++) {
          status.bms_data.temperatures_72v[i] = (int8_t)frame.data[i];
        }
        status.bms_data.updated = true;
        return false;
      }
      break;

    case 0x739: // 72V 总数据
      if (frame.can_dlc >= 8) {
        status.bms_data.total_voltage_72v = ((frame.data[0] << 8) | frame.data[1]) * 0.1f;
        status.bms_data.total_current_72v = ((int16_t)((frame.data[2] << 8) | frame.data[3])) * 0.1f;
        status.bms_data.remain_capacity_72v = ((frame.data[4] << 8) | frame.data[5]) * 0.1f;
        status.bms_data.full_capacity_72v = ((frame.data[6] << 8) | frame.data[7]) * 0.1f;
        status.bms_data.updated = true;
        return true;
      }
      break;
      
    case 0x73A: // 72V SOC/循环
      if (frame.can_dlc >= 8) {
        status.bms_data.soc_72v = ((frame.data[0] << 8) | frame.data[1]) * 0.1f;
        status.bms_data.cycle_count_72v = (frame.data[4] << 8) | frame.data[5];
        status.bms_data.updated = true;
        return false;
      }
      break;
      
    case 0x743: // 72V 保护状态
      if (frame.can_dlc >= 8) {
        status.bms_data.protection_status_72v = (frame.data[0] << 24) | (frame.data[1] << 16) | 
                                            (frame.data[2] << 8) | frame.data[3];
        status.bms_data.mos_charge_state_72v = (frame.data[3] & 0x01) != 0;
        status.bms_data.mos_discharge_state_72v = (frame.data[3] & 0x02) != 0;
        status.bms_data.updated = true;
        return true;
      }
      break;
      
    // ---- 电池开关状态解析 ----
    case 0x726: // 处理单片机上报的真实开关状态 (代替不存在的 0x900)
      if (frame.can_dlc >= 8) {
        status.bms_data.switch_state_12v = (frame.data[0] != 0);
        status.bms_data.switch_state_24v = (frame.data[1] != 0);
        status.bms_data.switch_state_72v = (frame.data[2] != 0); // 注意：单片机内部已经取反，这里只需判非0即可
        status.bms_data.switch_state_48v = status.bms_data.switch_state_72v; // 逻辑同步
        status.bms_data.updated = true;
        return true;
      }
      break;
      
    case 0x725:
    case 0x745: // 原0x825
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
    
    RCLCPP_INFO(this->get_logger(), "PowerHalNode constructor called");
    RCLCPP_INFO(this->get_logger(), "Simulation mode: %s", simulation_mode_ ? "ENABLED" : "DISABLED");
    
    // 初始化电池状态 (默认关闭)
    battery_states_[hal::BatteryType::BATTERY_12V] = false;
    battery_states_[hal::BatteryType::BATTERY_24V] = false;
    battery_states_[hal::BatteryType::BATTERY_72V] = false;
  }

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_configure(const rclcpp_lifecycle::State &)
  {
    RCLCPP_INFO(this->get_logger(), "Configuring PowerHalNode...");
    
    if (!simulation_mode_) {
      if (!can_interface_.init()) {
        RCLCPP_ERROR(this->get_logger(), "Failed to initialize CAN interface");
        return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::FAILURE;
      }
      RCLCPP_INFO(this->get_logger(), "CAN interface initialized successfully");
    }
    
    battery_pub_ = this->create_publisher<hal::msg::HalBattery>("/hal/battery", 10);
    
    battery_control_srv_ = this->create_service<hal::srv::HalBatteryControlSrv>(
      "/hal/batterycontrol",
      std::bind(&PowerHalNode::on_battery_control_service, this, 
                std::placeholders::_1, std::placeholders::_2));
    
    timer_ = this->create_wall_timer(
      100ms, std::bind(&PowerHalNode::on_timer, this));
    
    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
  }

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_activate(const rclcpp_lifecycle::State &)
  {
    timer_->reset();
    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
  }

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_deactivate(const rclcpp_lifecycle::State &)
  {
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
    if (simulation_mode_) {
      publish_simulated_status();
    } else {
      hal::BatteryStatusData status;
      while (can_interface_.receive_status(status)) {
        last_comm_time_ = this->now();
        check_battery_anomalies(status);
        
        // 实时更新本地开关状态变量（从底层反馈中获取）
        battery_states_[hal::BatteryType::BATTERY_12V] = status.bms_data.switch_state_12v;
        battery_states_[hal::BatteryType::BATTERY_24V] = status.bms_data.switch_state_24v;
        battery_states_[hal::BatteryType::BATTERY_72V] = status.bms_data.switch_state_72v;

        auto msg = create_battery_msg(status);
        battery_pub_->publish(msg);
        log_battery_status(msg);
      }
      
      check_communication_timeout();
    }
  }
  
  void check_battery_anomalies(const hal::BatteryStatusData& status)
  {
    if (status.bms_data.total_voltage_48v < voltage_threshold_ && status.bms_data.total_voltage_48v > 1.0f) {
      enter_safe_state("48V电池电压过低");
    }
    if (std::abs(status.bms_data.total_current_48v) > current_threshold_) {
      enter_safe_state("48V电池电流过大");
    }
    if (status.bms_data.max_temp_48v > temperature_threshold_) {
      enter_safe_state("48V电池温度过高");
    }
    if (status.bms_data.protection_status_48v != 0) {
      enter_safe_state("48V电池保护状态触发");
    }
    if (status.bms_data.total_voltage_72v > 1.0f && status.bms_data.total_voltage_72v < voltage_threshold_) {
      enter_safe_state("72V电池电压过低");
    }
    if (status.bms_data.total_current_72v != 0 && std::abs(status.bms_data.total_current_72v) > current_threshold_) {
      enter_safe_state("72V电池电流过大");
    }
    if (status.bms_data.max_temp_72v > temperature_threshold_) {
      enter_safe_state("72V电池温度过高");
    }
    if (status.bms_data.protection_status_72v != 0) {
      enter_safe_state("72V电池保护状态触发");
    }
  }
  
  void check_communication_timeout()
  {
    auto now = this->now();
    auto time_since_last_comm = now - last_comm_time_;
    
    if (time_since_last_comm.seconds() > communication_timeout_) {
      enter_safe_state("通信超时");
    }
  }
  
  void enter_safe_state(const std::string& reason)
  {
    if (!safe_state_) {
      safe_state_ = true;
      RCLCPP_WARN(this->get_logger(), "%s进入安全状态: %s%s", COLOR_YELLOW, reason.c_str(), COLOR_RESET);
      
      // 安全状态强制全关
      can_interface_.send_control_command(false, false, false);
      battery_states_[hal::BatteryType::BATTERY_12V] = false;
      battery_states_[hal::BatteryType::BATTERY_24V] = false;
      battery_states_[hal::BatteryType::BATTERY_72V] = false;
    }
  }
  
  void exit_safe_state()
  {
    if (safe_state_) {
      safe_state_ = false;
      RCLCPP_INFO(this->get_logger(), "%s退出安全状态%s", COLOR_GREEN, COLOR_RESET);
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
  
  void publish_simulated_status()
  {
    static int counter = 0;
    counter++;
    
    // 每10个周期发布一次状态 (100ms * 10 = 1s)
    if (counter % 10 != 0) return;
    
    // 显示各电池的开关状态
    RCLCPP_INFO(this->get_logger(), "%s[模拟模式]电池开关状态:%s", COLOR_CYAN, COLOR_RESET);
    for (const auto& [battery_type, switch_state] : battery_states_) {
      std::string color = switch_state ? COLOR_GREEN : COLOR_RED;
      std::string state_str = switch_state ? "开启" : "关闭";
      RCLCPP_INFO(this->get_logger(), 
          "%s  %s: %s%s",
          color.c_str(),
          get_battery_name(battery_type).c_str(),
          state_str.c_str(),
          COLOR_RESET);
    }
    
    // 发布电池状态信息
    auto msg = hal::msg::HalBattery();
    
    // 设置时间戳
    msg.timestamp = this->now().nanoseconds() / 1000000;  // 转换为毫秒
    
    // 填充48V电池数据
    msg.battery_status_48v = 0;  // 正常状态
    msg.battery_voltage_48v = 480;  // 48.0V
    msg.battery_current_48v = 50;  // 5.0A
    msg.cycle_count_48v = 100;  // 循环次数
    msg.battery_temperature_48v = 25;  // 25℃
    msg.remain_capacity_48v = 8500;  // 850.0AH
    msg.total_capacity_48v = 10000;  // 1000.0AH
    
    // 填充72V电池数据
    msg.battery_status_72v = 0;  // 正常状态
    msg.battery_voltage_72v = 720;  // 72.0V
    msg.battery_current_72v = 30;  // 3.0A
    msg.cycle_count_72v = 50;  // 循环次数
    msg.battery_temperature_72v = 28;  // 28℃
    msg.remain_capacity_72v = 17000;  // 1700.0AH
    msg.total_capacity_72v = 20000;  // 2000.0AH
    
    // 填充开关状态
    msg.switch_state_12v = battery_states_[hal::BatteryType::BATTERY_12V] ? 1 : 0;
    msg.switch_state_24v = battery_states_[hal::BatteryType::BATTERY_24V] ? 1 : 0;
    msg.switch_state_72v = battery_states_[hal::BatteryType::BATTERY_72V] ? 1 : 0;
    
    battery_pub_->publish(msg);
    
    // 显示电池状态信息
    RCLCPP_INFO(this->get_logger(), "%s[模拟模式]电池状态信息:%s", COLOR_CYAN, COLOR_RESET);
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
  }
  
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
  }

  std::string get_battery_name(hal::BatteryType battery_type) {
    switch (battery_type) {
      case hal::BatteryType::BATTERY_12V: return "12V电池";
      case hal::BatteryType::BATTERY_24V: return "24V电池";
      case hal::BatteryType::BATTERY_72V: return "72V电池";
      default: return "未知电池";
    }
  }

  void on_battery_control_service(
    const std::shared_ptr<hal::srv::HalBatteryControlSrv::Request> request,
    std::shared_ptr<hal::srv::HalBatteryControlSrv::Response> response) 
  {
    RCLCPP_INFO(this->get_logger(), "收到电池控制服务请求: command=%d", request->command);
    
    if (safe_state_) {
      response->success = false;
      response->message = "系统处于安全状态，无法执行控制指令";
      return;
    }
    
    if (request->command == 99) {
      enter_safe_state("急停指令");
      response->success = true;
      response->message = "急停指令执行成功";
      return;
    }
    
    bool success = false;
    std::string message = "";
    
    switch (request->command) {
      case 1: success = control_battery(hal::BatteryType::BATTERY_12V, true, "12V"); message = success ? "12V电池已开启" : "12V电池开启失败"; break;
      case 2: success = control_battery(hal::BatteryType::BATTERY_12V, false, "12V"); message = success ? "12V电池已关闭" : "12V电池关闭失败"; break;
      case 3: success = control_battery(hal::BatteryType::BATTERY_24V, true, "24V"); message = success ? "24V电池已开启" : "24V电池开启失败"; break;
      case 4: success = control_battery(hal::BatteryType::BATTERY_24V, false, "24V"); message = success ? "24V电池已关闭" : "24V电池关闭失败"; break;
      case 5: success = control_battery(hal::BatteryType::BATTERY_72V, true, "72V"); message = success ? "72V电池已开启" : "72V电池开启失败"; break;
      case 6: success = control_battery(hal::BatteryType::BATTERY_72V, false, "72V"); message = success ? "72V电池已关闭" : "72V电池关闭失败"; break;
      case 98: exit_safe_state(); response->success = true; response->message = "已退出安全状态"; return;
      default: success = false; message = "无效的控制命令"; break;
    }
    
    response->success = success;
    response->message = message;
  }
  
  // 在下发时集合发送 3 个通道的状态
  bool control_battery(hal::BatteryType battery_type, bool turn_on, const std::string& name)
  {
    bool old_state = battery_states_[battery_type];
    battery_states_[battery_type] = turn_on;

    if (simulation_mode_) {
      RCLCPP_INFO(this->get_logger(), "[模拟模式] 控制指令执行: 【%s】-> %s", name.c_str(), turn_on ? "开启" : "关闭");
      return true;
    } else {
      // 通过底层发送当前所有的状态
      if (can_interface_.send_control_command(
            battery_states_[hal::BatteryType::BATTERY_12V],
            battery_states_[hal::BatteryType::BATTERY_24V],
            battery_states_[hal::BatteryType::BATTERY_72V])) {
        
        RCLCPP_INFO(this->get_logger(), "控制指令执行: 【%s】-> %s", name.c_str(), turn_on ? "开启" : "关闭");
        return true;
      } else {
        // 如果发送失败，则撤销缓存的状态修改
        battery_states_[battery_type] = old_state;
        RCLCPP_ERROR(this->get_logger(), "发送【%s】控制指令失败", name.c_str());
        return false;
      }
    }
  }
  
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<hal::msg::HalBattery>::SharedPtr battery_pub_;
  rclcpp::Service<hal::srv::HalBatteryControlSrv>::SharedPtr battery_control_srv_;
  hal::CanInterface can_interface_;
  std::map<hal::BatteryType, bool> battery_states_;
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