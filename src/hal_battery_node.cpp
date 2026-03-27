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
#include "hal/msg/hal_battery_msg.hpp"
#include "hal/srv/hal_battery_control_srv.hpp"

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
  float total_voltage;      // 总电压
  float total_current;      // 总电流
  float remain_capacity;    // 剩余容量
  float full_capacity;      // 总容量
  float soc;                // 荷电状态
  uint16_t cycle_count;     // 循环次数
  float max_cell_vol;       // 最高单体电压
  uint8_t max_cell_id;      // 最高单体电压ID
  float min_cell_vol;       // 最低单体电压
  uint8_t min_cell_id;      // 最低单体电压ID
  int8_t max_temp;          // 最高温度
  int8_t min_temp;          // 最低温度
  uint8_t cell_count;       // 单体数量
  uint32_t protection_status; // 保护状态
  bool mos_charge_state;    // 充电MOS状态
  bool mos_discharge_state; // 放电MOS状态
  int8_t temperatures[8];   // 温度数组
  uint16_t cell_voltages[32]; // 单体电压数组
  bool updated;             // 数据更新标志
};

// 电池状态数据结构
enum class BatteryType {
  BATTERY_12V = 0,
  BATTERY_24V = 1,
  BATTERY_72V = 2
};

struct BatteryStatusData {
  float voltage;            // 电池电压
  uint8_t switch_state;     // 开关状态
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
  
  if (can_id >= 0x710 && can_id <= 0x718) {
    uint8_t group_idx = can_id - 0x710;
    uint8_t start_idx = group_idx * 4;
    if (start_idx + 3 < 32 && frame.can_dlc >= 8) {
      status.bms_data.cell_voltages[start_idx] = (frame.data[0] << 8) | frame.data[1];
      status.bms_data.cell_voltages[start_idx + 1] = (frame.data[2] << 8) | frame.data[3];
      status.bms_data.cell_voltages[start_idx + 2] = (frame.data[4] << 8) | frame.data[5];
      status.bms_data.cell_voltages[start_idx + 3] = (frame.data[6] << 8) | frame.data[7];
      status.bms_data.updated = true;
      return false;
    }
  }
  
  switch (can_id) {
    case 0x719:
      if (frame.can_dlc >= 8) {
        status.bms_data.total_voltage = ((frame.data[0] << 8) | frame.data[1]) * 0.1f;
        status.bms_data.total_current = ((int16_t)((frame.data[2] << 8) | frame.data[3])) * 0.1f;
        status.bms_data.remain_capacity = ((frame.data[4] << 8) | frame.data[5]) * 0.1f;
        status.bms_data.full_capacity = ((frame.data[6] << 8) | frame.data[7]) * 0.1f;
        status.voltage = status.bms_data.total_voltage;
        status.bms_data.updated = true;
        return true;
      }
      break;
      
    case 0x71A:
      if (frame.can_dlc >= 8) {
        status.bms_data.soc = ((frame.data[0] << 8) | frame.data[1]) * 0.1f;
        status.bms_data.cycle_count = (frame.data[4] << 8) | frame.data[5];
        status.bms_data.updated = true;
        return false;
      }
      break;
      
    case 0x71B:
      if (frame.can_dlc >= 8) {
        status.bms_data.max_cell_vol = ((frame.data[0] << 8) | frame.data[1]) * 0.001f;
        status.bms_data.max_cell_id = frame.data[2];
        status.bms_data.min_cell_vol = ((frame.data[3] << 8) | frame.data[4]) * 0.001f;
        status.bms_data.min_cell_id = frame.data[5];
        status.bms_data.updated = true;
        return false;
      }
      break;
      
    case 0x71C:
      if (frame.can_dlc >= 8) {
        status.bms_data.max_temp = (int8_t)frame.data[0];
        status.bms_data.min_temp = (int8_t)frame.data[4];
        status.bms_data.updated = true;
        return false;
      }
      break;
      
    case 0x71E:
      if (frame.can_dlc >= 8) {
        status.bms_data.cell_count = frame.data[3];
        status.bms_data.updated = true;
        return false;
      }
      break;
      
    case 0x723:
      if (frame.can_dlc >= 8) {
        status.bms_data.protection_status = (frame.data[0] << 24) | (frame.data[1] << 16) | 
                                            (frame.data[2] << 8) | frame.data[3];
        status.bms_data.mos_charge_state = (frame.data[3] & 0x01) != 0;
        status.bms_data.mos_discharge_state = (frame.data[3] & 0x02) != 0;
        status.switch_state = status.bms_data.mos_discharge_state;
        status.bms_data.updated = true;
        return true;
      }
      break;
      
    case 0x724:
      if (frame.can_dlc >= 8) {
        for(int i = 0; i < 8; i++) {
          status.bms_data.temperatures[i] = (int8_t)frame.data[i];
        }
        status.bms_data.updated = true;
        return false;
      }
      break;
      
    case 0x725:
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
    simulation_mode_(false)
  {
    this->declare_parameter("simulation_mode", false);
    simulation_mode_ = this->get_parameter("simulation_mode").as_bool();
    
    RCLCPP_INFO(this->get_logger(), "PowerHalNode constructor called");
    RCLCPP_INFO(this->get_logger(), "Simulation mode: %s", simulation_mode_ ? "ENABLED" : "DISABLED");
    
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
    
    // 创建电池状态发布者 (hal_battery_msg)
    battery_pub_ = this->create_publisher<hal::msg::HalBatteryMsg>(
      "/hal/battery_msg", 10);
    
    // 创建电池控制服务 (hal_batterycontrol_srv)
    battery_control_srv_ = this->create_service<hal::srv::HalBatteryControlSrv>(
      "/hal/batterycontrol_srv",
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
        // 处理从CAN接收到的数据并发布
        auto msg = create_battery_msg(status);
        battery_pub_->publish(msg);
        log_battery_status(msg);
      }
    }
  }
  
  hal::msg::HalBatteryMsg create_battery_msg(const hal::BatteryStatusData& status)
  {
    auto msg = hal::msg::HalBatteryMsg();
    
    // 根据CAN数据填充消息
    msg.battery_status = 0;  // 需要根据实际状态设置
    msg.battery_current = static_cast<int16_t>(status.bms_data.total_current * 10);  // 转换为0.1A
    msg.cycle_count = status.bms_data.cycle_count;
    msg.remain_capacity = static_cast<uint16_t>(status.bms_data.remain_capacity * 10);  // 转换为0.1AH
    msg.total_capacity = static_cast<uint16_t>(status.bms_data.full_capacity * 10);  // 转换为0.1AH
    msg.switch_state = status.switch_state ? 1 : 0;
    
    return msg;
  }
  
  void publish_simulated_status()
  {
    static int counter = 0;
    counter++;
    
    // 每10个周期发布一次状态
    if (counter % 10 != 0) return;
    
    for (const auto& [battery_type, switch_state] : battery_states_) {
      auto msg = hal::msg::HalBatteryMsg();
      auto& data = battery_data_[battery_type];
      
      msg.battery_status = switch_state ? 1 : 0;
      msg.battery_current = switch_state ? static_cast<int16_t>(50) : 0;  // 5.0A = 50 * 0.1A
      msg.cycle_count = data.cycle_count;
      msg.remain_capacity = switch_state ? static_cast<uint16_t>(data.total_capacity * 0.85) : 0;  // 85%电量
      msg.total_capacity = data.total_capacity;
      msg.switch_state = switch_state ? 1 : 0;
      
      battery_pub_->publish(msg);
      
      std::string color = switch_state ? COLOR_GREEN : COLOR_RED;
      std::string state_str = switch_state ? "开启" : "关闭";
      
      RCLCPP_INFO(this->get_logger(), 
          "%s[模拟模式]【%s】%s%s 电流:%.1fA 电量:%.1f/%.1fAH 循环:%d%s",
          color.c_str(),
          get_battery_name(battery_type).c_str(),
          state_str.c_str(),
          COLOR_RESET,
          msg.battery_current / 10.0,
          msg.remain_capacity / 10.0,
          msg.total_capacity / 10.0,
          msg.cycle_count,
          COLOR_RESET);
    }
  }
  
  void log_battery_status(const hal::msg::HalBatteryMsg& msg)
  {
    std::string color = msg.switch_state ? COLOR_GREEN : COLOR_RED;
    std::string state_str = msg.switch_state ? "开启" : "关闭";
    
    RCLCPP_INFO(this->get_logger(), 
        "%s【电池】%s%s 电流:%.1fA 电量:%.1f/%.1fAH 循环:%d%s",
        color.c_str(),
        state_str.c_str(),
        COLOR_RESET,
        msg.battery_current / 10.0,
        msg.remain_capacity / 10.0,
        msg.total_capacity / 10.0,
        msg.cycle_count,
        COLOR_RESET);
  }
  
  void on_battery_control_service(
    const std::shared_ptr<hal::srv::HalBatteryControlSrv::Request> request,
    std::shared_ptr<hal::srv::HalBatteryControlSrv::Response> response) 
  {
    RCLCPP_INFO(this->get_logger(), "收到电池控制服务请求: command=%d", request->command);
    
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
  rclcpp::Publisher<hal::msg::HalBatteryMsg>::SharedPtr battery_pub_;
  rclcpp::Service<hal::srv::HalBatteryControlSrv>::SharedPtr battery_control_srv_;
  hal::CanInterface can_interface_;
  std::map<hal::BatteryType, bool> battery_states_;
  std::map<hal::BatteryType, BatteryData> battery_data_;
  bool simulation_mode_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  
  auto node = std::make_shared<PowerHalNode>("hal_battery_node");
  
  rclcpp::spin(node->get_node_base_interface());
  
  rclcpp::shutdown();
  return 0;
}