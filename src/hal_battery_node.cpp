#include <memory>
#include <string>
#include <map>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include <lifecycle_msgs/msg/state.hpp>
#include <lifecycle_msgs/msg/transition.hpp>
#include "power_hal/can_interface.hpp"
#include "power_hal/msg/hal_battery_msg.hpp"
#include "power_hal/srv/hal_battery_control_srv.hpp"

#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[31m"
#define COLOR_GREEN   "\033[32m"
#define COLOR_YELLOW  "\033[33m"
#define COLOR_BLUE    "\033[34m"
#define COLOR_CYAN    "\033[36m"

using namespace std::chrono_literals;

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
    battery_states_[power_hal::BatteryType::BATTERY_12V] = false;
    battery_states_[power_hal::BatteryType::BATTERY_24V] = false;
    battery_states_[power_hal::BatteryType::BATTERY_72V] = false;
    
    // 初始化模拟数据
    battery_data_[power_hal::BatteryType::BATTERY_12V] = {0, 0, 0, 0, 500, 0};
    battery_data_[power_hal::BatteryType::BATTERY_24V] = {0, 0, 0, 0, 1000, 0};
    battery_data_[power_hal::BatteryType::BATTERY_72V] = {0, 0, 0, 0, 2000, 0};
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
    battery_pub_ = this->create_publisher<power_hal::msg::HalBatteryMsg>(
      "/hal/battery_msg", 10);
    
    // 创建电池控制服务 (hal_batterycontrol_srv)
    battery_control_srv_ = this->create_service<power_hal::srv::HalBatteryControlSrv>(
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
      power_hal::BatteryStatusData status;
      while (can_interface_.receive_status(status)) {
        // 处理从CAN接收到的数据并发布
        auto msg = create_battery_msg(status);
        battery_pub_->publish(msg);
        log_battery_status(msg);
      }
    }
  }
  
  power_hal::msg::HalBatteryMsg create_battery_msg(const power_hal::BatteryStatusData& status)
  {
    auto msg = power_hal::msg::HalBatteryMsg();
    
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
      auto msg = power_hal::msg::HalBatteryMsg();
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
  
  void log_battery_status(const power_hal::msg::HalBatteryMsg& msg)
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
    const std::shared_ptr<power_hal::srv::HalBatteryControlSrv::Request> request,
    std::shared_ptr<power_hal::srv::HalBatteryControlSrv::Response> response) 
  {
    RCLCPP_INFO(this->get_logger(), "收到电池控制服务请求: command=%d", request->command);
    
    bool success = false;
    std::string message = "";
    
    switch (request->command) {
      case 1: // 12V开
        success = control_battery(power_hal::BatteryType::BATTERY_12V, true, "12V");
        message = success ? "12V电池已开启" : "12V电池开启失败";
        break;
        
      case 2: // 12V关
        success = control_battery(power_hal::BatteryType::BATTERY_12V, false, "12V");
        message = success ? "12V电池已关闭" : "12V电池关闭失败";
        break;
        
      case 3: // 24V开
        success = control_battery(power_hal::BatteryType::BATTERY_24V, true, "24V");
        message = success ? "24V电池已开启" : "24V电池开启失败";
        break;
        
      case 4: // 24V关
        success = control_battery(power_hal::BatteryType::BATTERY_24V, false, "24V");
        message = success ? "24V电池已关闭" : "24V电池关闭失败";
        break;
        
      case 5: // 72V开
        success = control_battery(power_hal::BatteryType::BATTERY_72V, true, "72V");
        message = success ? "72V电池已开启" : "72V电池开启失败";
        break;
        
      case 6: // 72V关
        success = control_battery(power_hal::BatteryType::BATTERY_72V, false, "72V");
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
  
  bool control_battery(power_hal::BatteryType battery_type, bool turn_on, const std::string& name)
  {
    if (simulation_mode_) {
      battery_states_[battery_type] = turn_on;
      RCLCPP_INFO(this->get_logger(), 
                   "[模拟模式] 控制指令执行: 【%s】-> %s",
                   name.c_str(),
                   turn_on ? "开启" : "关闭");
      return true;
    } else {
      power_hal::BatteryControlData control;
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
  
  std::string get_battery_name(power_hal::BatteryType battery_type) {
    switch (battery_type) {
      case power_hal::BatteryType::BATTERY_12V: return "12V电池";
      case power_hal::BatteryType::BATTERY_24V: return "24V电池";
      case power_hal::BatteryType::BATTERY_72V: return "72V电池";
      default: return "未知电池";
    }
  }
  
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Publisher<power_hal::msg::HalBatteryMsg>::SharedPtr battery_pub_;
  rclcpp::Service<power_hal::srv::HalBatteryControlSrv>::SharedPtr battery_control_srv_;
  power_hal::CanInterface can_interface_;
  std::map<power_hal::BatteryType, bool> battery_states_;
  std::map<power_hal::BatteryType, BatteryData> battery_data_;
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
