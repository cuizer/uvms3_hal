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
  float total_voltage_48v;      // 总电压
  float total_current_48v;      // 总电流
  float remain_capacity_48v;    // 剩余容量
  float full_capacity_48v;      // 总容量
  float soc_48v;                // 荷电状态
  uint16_t cycle_count_48v;     // 循环次数
  float max_cell_vol_48v;       // 最高单体电压
  uint8_t max_cell_id_48v;      // 最高单体电压ID
  float min_cell_vol_48v;       // 最低单体电压
  uint8_t min_cell_id_48v;      // 最低单体电压ID
  int8_t max_temp_48v;          // 最高温度
  int8_t min_temp_48v;          // 最低温度
  uint8_t cell_count_48v;       // 单体数量
  uint32_t protection_status_48v; // 保护状态
  bool mos_charge_state_48v;    // 充电MOS状态
  bool mos_discharge_state_48v; // 放电MOS状态
  int8_t temperatures_48v[8];   // 温度数组
  uint16_t cell_voltages_48v[32]; // 单体电压数组
  
  // 72V电池数据
  float total_voltage_72v;      // 总电压
  float total_current_72v;      // 总电流
  float remain_capacity_72v;    // 剩余容量
  float full_capacity_72v;      // 总容量
  float soc_72v;                // 荷电状态
  uint16_t cycle_count_72v;     // 循环次数
  float max_cell_vol_72v;       // 最高单体电压
  uint8_t max_cell_id_72v;      // 最高单体电压ID
  float min_cell_vol_72v;       // 最低单体电压
  uint8_t min_cell_id_72v;      // 最低单体电压ID
  int8_t max_temp_72v;          // 最高温度
  int8_t min_temp_72v;          // 最低温度
  uint8_t cell_count_72v;       // 单体数量
  uint32_t protection_status_72v; // 保护状态
  bool mos_charge_state_72v;    // 充电MOS状态
  bool mos_discharge_state_72v; // 放电MOS状态
  int8_t temperatures_72v[8];   // 温度数组
  uint16_t cell_voltages_72v[32]; // 单体电压数组
  
  // 12V和24V电池数据
  uint16_t voltage_12v;         // 12V电池电压
  uint16_t voltage_24v;         // 24V电池电压
  uint8_t temperature_12v;      // 12V电池温度
  uint8_t temperature_24v;      // 24V电池温度
  
  // 开关状态
  bool switch_state_12v;        // 12V电池开关状态
  bool switch_state_24v;        // 24V电池开关状态
  bool switch_state_48v;        // 48V电池开关状态
  bool switch_state_72v;        // 72V电池开关状态
  
  bool updated;                 // 数据更新标志
};

// 电池状态数据结构
enum class BatteryType {
  BATTERY_12V = 0,
  BATTERY_24V = 1,
  BATTERY_72V = 2
};

struct BatteryStatusData {
  BmsData bms_data;         // BMS数据
};

struct BatteryControlData {
  BatteryType battery_type; // 电池类型
  bool switch_state;        // 开关状态
};

// CAN接口类
class CanInterface {
public:
  CanInterface(const std::string & can_interface);
  ~CanInterface();
  
  bool init();
  void close();
  bool send_control_command(const BatteryControlData & control);
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

bool CanInterface::send_control_command(const BatteryControlData & control)
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
  frame.data[2] = static_cast<uint8_t>(control.battery_type);
  frame.data[3] = control.switch_state ? 0x01 : 0x00;
  frame.data[4] = 0;
  frame.data[5] = 0;
  frame.data[6] = 0;
  frame.data[7] = 0;

  ssize_t nbytes = write(socket_fd_, &frame, sizeof(struct can_frame));
  if (nbytes != sizeof(struct can_frame)) {
    RCLCPP_ERROR(rclcpp::get_logger("CanInterface"), "Failed to send CAN frame");
    return false;
  }

  RCLCPP_INFO(rclcpp::get_logger("CanInterface"), 
              "Sent control command: Battery %d, State %s", 
              static_cast<int>(control.battery_type),
              control.switch_state ? "ON" : "OFF");
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
  
  // 处理48V电池数据 (CAN ID 0x710-0x725)
  if (can_id >= 0x710 && can_id <= 0x718) {
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
  
  switch (can_id) {
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
        status.bms_data.switch_state_48v = status.bms_data.mos_discharge_state_48v;
        status.bms_data.updated = true;
        return true;
      }
      break;
      
    case 0x724:
      if (frame.can_dlc >= 8) {
        for(int i = 0; i < 8; i++) {
          status.bms_data.temperatures_48v[i] = (int8_t)frame.data[i];
        }
        status.bms_data.updated = true;
        return false;
      }
      break;
      
    // 处理72V电池数据 (CAN ID 0x810-0x825)
    case 0x819:
      if (frame.can_dlc >= 8) {
        status.bms_data.total_voltage_72v = ((frame.data[0] << 8) | frame.data[1]) * 0.1f;
        status.bms_data.total_current_72v = ((int16_t)((frame.data[2] << 8) | frame.data[3])) * 0.1f;
        status.bms_data.remain_capacity_72v = ((frame.data[4] << 8) | frame.data[5]) * 0.1f;
        status.bms_data.full_capacity_72v = ((frame.data[6] << 8) | frame.data[7]) * 0.1f;
        status.bms_data.updated = true;
        return true;
      }
      break;
      
    case 0x81A:
      if (frame.can_dlc >= 8) {
        status.bms_data.soc_72v = ((frame.data[0] << 8) | frame.data[1]) * 0.1f;
        status.bms_data.cycle_count_72v = (frame.data[4] << 8) | frame.data[5];
        status.bms_data.updated = true;
        return false;
      }
      break;
      
    case 0x823:
      if (frame.can_dlc >= 8) {
        status.bms_data.protection_status_72v = (frame.data[0] << 24) | (frame.data[1] << 16) | 
                                            (frame.data[2] << 8) | frame.data[3];
        status.bms_data.mos_charge_state_72v = (frame.data[3] & 0x01) != 0;
        status.bms_data.mos_discharge_state_72v = (frame.data[3] & 0x02) != 0;
        status.bms_data.switch_state_72v = status.bms_data.mos_discharge_state_72v;
        status.bms_data.updated = true;
        return true;
      }
      break;
      
    // 处理12V和24V电池数据 (CAN ID 0x900)
    case 0x900:
      if (frame.can_dlc >= 8) {
        status.bms_data.voltage_12v = (frame.data[0] << 8) | frame.data[1];
        status.bms_data.voltage_24v = (frame.data[2] << 8) | frame.data[3];
        status.bms_data.temperature_12v = frame.data[4];
        status.bms_data.temperature_24v = frame.data[5];
        status.bms_data.switch_state_12v = (frame.data[6] & 0x01) != 0;
        status.bms_data.switch_state_24v = (frame.data[6] & 0x02) != 0;
        status.bms_data.updated = true;
        return true;
      }
      break;
      
    case 0x725:
    case 0x825:
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
    RCLCPP_INFO(this->get_logger(), "Voltage threshold: %.1fV", voltage_threshold_);
    RCLCPP_INFO(this->get_logger(), "Current threshold: %.1fA", current_threshold_);
    RCLCPP_INFO(this->get_logger(), "Temperature threshold: %.1fC", temperature_threshold_);
    RCLCPP_INFO(this->get_logger(), "Communication timeout: %.1fs", communication_timeout_);
    
    // 初始化电池状态
    battery_states_[hal::BatteryType::BATTERY_12V] = false;
    battery_states_[hal::BatteryType::BATTERY_24V] = false;
    battery_states_[hal::BatteryType::BATTERY_72V] = false;
    
    // 初始化模拟数据
    battery_data_[hal::BatteryType::BATTERY_12V] = {0, 0, 0, 0, 500, 0};
    battery_data_[hal::BatteryType::BATTERY_24V] = {0, 0, 0, 0, 1000, 0};
    battery_data_[hal::BatteryType::BATTERY_72V] = {0, 0, 0, 0, 2000, 0};
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
    } else {
      RCLCPP_INFO(this->get_logger(), "Running in simulation mode - CAN interface not initialized");
    }
    
    // 创建电池状态发布者 (hal_battery)
    battery_pub_ = this->create_publisher<hal::msg::HalBattery>(
      "/hal/battery", 10);
    
    // 创建电池控制服务 (hal_batterycontrol)
    battery_control_srv_ = this->create_service<hal::srv::HalBatteryControlSrv>(
      "/hal/batterycontrol",
      std::bind(&PowerHalNode::on_battery_control_service, this, 
                std::placeholders::_1, std::placeholders::_2));
    
    // 创建定时器，定期发布电池状态
    timer_ = this->create_wall_timer(
      100ms, std::bind(&PowerHalNode::on_timer, this));
    
    RCLCPP_INFO(this->get_logger(), "PowerHalNode configured successfully");
    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
  }

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_activate(const rclcpp_lifecycle::State &)
  {
    RCLCPP_INFO(this->get_logger(), "Activating PowerHalNode...");
    
    timer_->reset();
    
    RCLCPP_INFO(this->get_logger(), "PowerHalNode activated successfully");
    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
  }

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_deactivate(const rclcpp_lifecycle::State &)
  {
    RCLCPP_INFO(this->get_logger(), "Deactivating PowerHalNode...");
    
    timer_->cancel();
    
    RCLCPP_INFO(this->get_logger(), "PowerHalNode deactivated successfully");
    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
  }

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_cleanup(const rclcpp_lifecycle::State &)
  {
    RCLCPP_INFO(this->get_logger(), "Cleaning up PowerHalNode...");
    
    timer_.reset();
    battery_pub_.reset();
    battery_control_srv_.reset();
    can_interface_.close();
    
    RCLCPP_INFO(this->get_logger(), "PowerHalNode cleaned up successfully");
    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
  }

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_shutdown(const rclcpp_lifecycle::State &)
  {
    RCLCPP_INFO(this->get_logger(), "Shutting down PowerHalNode...");
    
    timer_.reset();
    
    RCLCPP_INFO(this->get_logger(), "PowerHalNode shut down successfully");
    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
  }

  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn
  on_error(const rclcpp_lifecycle::State &)
  {
    RCLCPP_INFO(this->get_logger(), "Error occurred in PowerHalNode");
    return rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn::SUCCESS;
  }

private:
  // 电池数据结构
  struct BatteryData {
    int8_t battery_status;      // 电池状态
    int16_t battery_current;    // 电池电流 (0.1A)
    uint16_t cycle_count;       // 循环次数
    uint16_t remain_capacity;   // 剩余电量 (0.1AH)
    uint16_t total_capacity;    // 总电量 (0.1AH)
    uint8_t switch_state;       // 开关状态
  };

  void on_timer()
  {
    if (simulation_mode_) {
      publish_simulated_status();
    } else {
      // 实际模式下从CAN接口读取数据
      hal::BatteryStatusData status;
      while (can_interface_.receive_status(status)) {
        // 更新通信时间戳
        last_comm_time_ = this->now();
        
        // 检查电池异常
        check_battery_anomalies(status);
        
        // 检查通信超时
        check_communication_timeout();
        
        // 处理从CAN接收到的数据并发布（只处理48V电池状态）
        auto msg = create_battery_msg(status);
        battery_pub_->publish(msg);
        log_battery_status(msg);
      }
      
      // 定期检查通信超时
      check_communication_timeout();
    }
  }
  
  void check_battery_anomalies(const hal::BatteryStatusData& status)
  {
    // 检查48V电池电压异常
    if (status.bms_data.total_voltage_48v < voltage_threshold_) {
      RCLCPP_WARN(this->get_logger(), 
                  "%s48V电池电压过低: %.1fV (阈值: %.1fV)%s",
                  COLOR_RED,
                  status.bms_data.total_voltage_48v,
                  voltage_threshold_,
                  COLOR_RESET);
      enter_safe_state("48V电池电压过低");
    }
    
    // 检查48V电池电流异常
    if (std::abs(status.bms_data.total_current_48v) > current_threshold_) {
      RCLCPP_WARN(this->get_logger(), 
                  "%s48V电池电流过大: %.1fA (阈值: %.1fA)%s",
                  COLOR_RED,
                  status.bms_data.total_current_48v,
                  current_threshold_,
                  COLOR_RESET);
      enter_safe_state("48V电池电流过大");
    }
    
    // 检查48V电池温度异常
    if (status.bms_data.max_temp_48v > temperature_threshold_) {
      RCLCPP_WARN(this->get_logger(), 
                  "%s48V电池温度过高: %d°C (阈值: %.1f°C)%s",
                  COLOR_RED,
                  status.bms_data.max_temp_48v,
                  temperature_threshold_,
                  COLOR_RESET);
      enter_safe_state("48V电池温度过高");
    }
    
    // 检查48V电池保护状态
    if (status.bms_data.protection_status_48v != 0) {
      RCLCPP_WARN(this->get_logger(), 
                  "%s48V电池保护状态触发: 0x%08X%s",
                  COLOR_RED,
                  status.bms_data.protection_status_48v,
                  COLOR_RESET);
      enter_safe_state("48V电池保护状态触发");
    }
    
    // 检查72V电池电压异常
    if (status.bms_data.total_voltage_72v > 0 && status.bms_data.total_voltage_72v < voltage_threshold_) {
      RCLCPP_WARN(this->get_logger(), 
                  "%s72V电池电压过低: %.1fV (阈值: %.1fV)%s",
                  COLOR_RED,
                  status.bms_data.total_voltage_72v,
                  voltage_threshold_,
                  COLOR_RESET);
      enter_safe_state("72V电池电压过低");
    }
    
    // 检查72V电池电流异常
    if (status.bms_data.total_current_72v != 0 && std::abs(status.bms_data.total_current_72v) > current_threshold_) {
      RCLCPP_WARN(this->get_logger(), 
                  "%s72V电池电流过大: %.1fA (阈值: %.1fA)%s",
                  COLOR_RED,
                  status.bms_data.total_current_72v,
                  current_threshold_,
                  COLOR_RESET);
      enter_safe_state("72V电池电流过大");
    }
    
    // 检查72V电池温度异常
    if (status.bms_data.max_temp_72v > temperature_threshold_) {
      RCLCPP_WARN(this->get_logger(), 
                  "%s72V电池温度过高: %d°C (阈值: %.1f°C)%s",
                  COLOR_RED,
                  status.bms_data.max_temp_72v,
                  temperature_threshold_,
                  COLOR_RESET);
      enter_safe_state("72V电池温度过高");
    }
    
    // 检查72V电池保护状态
    if (status.bms_data.protection_status_72v != 0) {
      RCLCPP_WARN(this->get_logger(), 
                  "%s72V电池保护状态触发: 0x%08X%s",
                  COLOR_RED,
                  status.bms_data.protection_status_72v,
                  COLOR_RESET);
      enter_safe_state("72V电池保护状态触发");
    }
  }
  
  void check_communication_timeout()
  {
    auto now = this->now();
    auto time_since_last_comm = now - last_comm_time_;
    
    if (time_since_last_comm.seconds() > communication_timeout_) {
      RCLCPP_WARN(this->get_logger(), 
                  "%s通信超时: %.1fs (阈值: %.1fs)%s",
                  COLOR_RED,
                  time_since_last_comm.seconds(),
                  communication_timeout_,
                  COLOR_RESET);
      enter_safe_state("通信超时");
    }
  }
  
  void enter_safe_state(const std::string& reason)
  {
    if (!safe_state_) {
      safe_state_ = true;
      RCLCPP_WARN(this->get_logger(), 
                  "%s进入安全状态: %s%s",
                  COLOR_YELLOW,
                  reason.c_str(),
                  COLOR_RESET);
      
      // 关闭所有电池
      for (auto& [battery_type, state] : battery_states_) {
        if (state) {
          control_battery(battery_type, false, get_battery_name(battery_type));
        }
      }
    }
  }
  
  void exit_safe_state()
  {
    if (safe_state_) {
      safe_state_ = false;
      RCLCPP_INFO(this->get_logger(), 
                  "%s退出安全状态%s",
                  COLOR_GREEN,
                  COLOR_RESET);
    }
  }
  
  hal::msg::HalBattery create_battery_msg(const hal::BatteryStatusData& status)
  {
    auto msg = hal::msg::HalBattery();
    
    // 设置时间戳
    msg.timestamp = this->now().nanoseconds() / 1000000;  // 转换为毫秒
    
    // 填充48V电池数据
    msg.battery_status_48v = status.bms_data.protection_status_48v != 0 ? 1 : 0;
    msg.battery_voltage_48v = static_cast<uint16_t>(status.bms_data.total_voltage_48v * 10);  // 转换为0.1V
    msg.battery_current_48v = static_cast<int16_t>(status.bms_data.total_current_48v * 10);  // 转换为0.1A
    msg.cycle_count_48v = status.bms_data.cycle_count_48v;
    msg.battery_temperature_48v = static_cast<uint16_t>(status.bms_data.max_temp_48v);
    msg.remain_capacity_48v = static_cast<uint16_t>(status.bms_data.remain_capacity_48v * 10);  // 转换为0.1AH
    msg.total_capacity_48v = static_cast<uint16_t>(status.bms_data.full_capacity_48v * 10);  // 转换为0.1AH
    
    // 填充72V电池数据
    msg.battery_status_72v = status.bms_data.protection_status_72v != 0 ? 1 : 0;
    msg.battery_voltage_72v = static_cast<uint16_t>(status.bms_data.total_voltage_72v * 10);  // 转换为0.1V
    msg.battery_current_72v = static_cast<int16_t>(status.bms_data.total_current_72v * 10);  // 转换为0.1A
    msg.cycle_count_72v = status.bms_data.cycle_count_72v;
    msg.battery_temperature_72v = static_cast<uint16_t>(status.bms_data.max_temp_72v);
    msg.remain_capacity_72v = static_cast<uint16_t>(status.bms_data.remain_capacity_72v * 10);  // 转换为0.1AH
    msg.total_capacity_72v = static_cast<uint16_t>(status.bms_data.full_capacity_72v * 10);  // 转换为0.1AH
    
    // 填充开关状态
    msg.switch_state_12v = status.bms_data.switch_state_12v ? 1 : 0;
    msg.switch_state_24v = status.bms_data.switch_state_24v ? 1 : 0;
    msg.switch_state_72v = status.bms_data.switch_state_72v ? 1 : 0;
    
    return msg;
  }
  
  void publish_simulated_status()
  {
    static int counter = 0;
    counter++;
    
    // 每10个周期发布一次状态
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
  
  void on_battery_control_service(
    const std::shared_ptr<hal::srv::HalBatteryControlSrv::Request> request,
    std::shared_ptr<hal::srv::HalBatteryControlSrv::Response> response) 
  {
    RCLCPP_INFO(this->get_logger(), "收到电池控制服务请求: command=%d", request->command);
    
    // 检查安全状态
    if (safe_state_) {
      response->success = false;
      response->message = "系统处于安全状态，无法执行控制指令";
      RCLCPP_WARN(this->get_logger(), "%s安全状态下拒绝控制指令: %d%s", 
                  COLOR_YELLOW, request->command, COLOR_RESET);
      return;
    }
    
    // 检查急停指令
    if (request->command == 99) { // 急停指令
      RCLCPP_INFO(this->get_logger(), "%s执行急停指令%s", COLOR_RED, COLOR_RESET);
      enter_safe_state("急停指令");
      response->success = true;
      response->message = "急停指令执行成功";
      return;
    }
    
    bool success = false;
    std::string message = "";
    
    switch (request->command) {
      case 1: // 12V开
        success = control_battery(hal::BatteryType::BATTERY_12V, true, "12V");
        message = success ? "12V电池已开启" : "12V电池开启失败";
        break;
        
      case 2: // 12V关
        success = control_battery(hal::BatteryType::BATTERY_12V, false, "12V");
        message = success ? "12V电池已关闭" : "12V电池关闭失败";
        break;
        
      case 3: // 24V开
        success = control_battery(hal::BatteryType::BATTERY_24V, true, "24V");
        message = success ? "24V电池已开启" : "24V电池开启失败";
        break;
        
      case 4: // 24V关
        success = control_battery(hal::BatteryType::BATTERY_24V, false, "24V");
        message = success ? "24V电池已关闭" : "24V电池关闭失败";
        break;
        
      case 5: // 72V开
        success = control_battery(hal::BatteryType::BATTERY_72V, true, "72V");
        message = success ? "72V电池已开启" : "72V电池开启失败";
        break;
        
      case 6: // 72V关
        success = control_battery(hal::BatteryType::BATTERY_72V, false, "72V");
        message = success ? "72V电池已关闭" : "72V电池关闭失败";
        break;
        
      case 98: // 退出安全状态
        exit_safe_state();
        response->success = true;
        response->message = "已退出安全状态";
        return;
        
      default:
        success = false;
        message = "无效的控制命令";
        break;
    }
    
    response->success = success;
    response->message = message;
    
    RCLCPP_INFO(this->get_logger(), "电池控制服务响应: success=%s, message=%s", 
                success ? "true" : "false", message.c_str());
  }
  
  bool control_battery(hal::BatteryType battery_type, bool turn_on, const std::string& name)
  {
    if (simulation_mode_) {
      battery_states_[battery_type] = turn_on;
      // 更新BmsData中的开关状态
      switch (battery_type) {
        case hal::BatteryType::BATTERY_12V:
          bms_data_.switch_state_12v = turn_on;
          break;
        case hal::BatteryType::BATTERY_24V:
          bms_data_.switch_state_24v = turn_on;
          break;
        case hal::BatteryType::BATTERY_72V:
          bms_data_.switch_state_72v = turn_on;
          bms_data_.switch_state_48v = turn_on;  // 48V开关状态跟随72V
          break;
      }
      RCLCPP_INFO(this->get_logger(), 
                   "[模拟模式] 控制指令执行: 【%s】-> %s",
                   name.c_str(),
                   turn_on ? "开启" : "关闭");
      return true;
    } else {
      hal::BatteryControlData control;
      control.battery_type = battery_type;
      control.switch_state = turn_on;
      
      if (can_interface_.send_control_command(control)) {
        battery_states_[battery_type] = turn_on;
        // 更新BmsData中的开关状态
        switch (battery_type) {
          case hal::BatteryType::BATTERY_12V:
            bms_data_.switch_state_12v = turn_on;
            break;
          case hal::BatteryType::BATTERY_24V:
            bms_data_.switch_state_24v = turn_on;
            break;
          case hal::BatteryType::BATTERY_72V:
            bms_data_.switch_state_72v = turn_on;
            bms_data_.switch_state_48v = turn_on;  // 48V开关状态跟随72V
            break;
        }
        RCLCPP_INFO(this->get_logger(), 
                   "控制指令执行: 【%s】-> %s",
                   name.c_str(),
                   turn_on ? "开启" : "关闭");
        return true;
      } else {
        RCLCPP_ERROR(this->get_logger(), 
                     "发送【%s】控制指令失败",
                     name.c_str());
        return false;
      }
    }
  }
  
  std::string get_battery_name(hal::BatteryType battery_type) {
    switch (battery_type) {
      case hal::BatteryType::BATTERY_12V: return "12V电池";
      case hal::BatteryType::BATTERY_24V: return "24V电池";
      case hal::BatteryType::BATTERY_72V: return "72V电池";
      default: return "未知电池";
    }
  }
  
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<hal::msg::HalBattery>::SharedPtr battery_pub_;
  rclcpp::Service<hal::srv::HalBatteryControlSrv>::SharedPtr battery_control_srv_;
  hal::CanInterface can_interface_;
  std::map<hal::BatteryType, bool> battery_states_;
  std::map<hal::BatteryType, BatteryData> battery_data_;
  hal::BmsData bms_data_;       // 电池状态数据
  bool simulation_mode_;
  bool safe_state_;            // 安全状态标志
  rclcpp::Time last_comm_time_; // 最后通信时间戳
  double voltage_threshold_;    // 电压阈值
  double current_threshold_;    // 电流阈值
  double temperature_threshold_; // 温度阈值
  double communication_timeout_; // 通信超时时间
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  
  auto node = std::make_shared<PowerHalNode>("hal_battery_node");
  
  rclcpp::spin(node->get_node_base_interface());
  
  rclcpp::shutdown();
  return 0;
}