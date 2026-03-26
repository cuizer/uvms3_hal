#include <memory>
#include <string>
#include <map>
#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include <lifecycle_msgs/msg/state.hpp>
#include <lifecycle_msgs/msg/transition.hpp>
#include "power_hal/can_interface.hpp"
#include "power_hal/msg/battery_control.hpp"
#include "power_hal/msg/battery_status.hpp"

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
    
    battery_states_[power_hal::BatteryType::BATTERY_12V] = false;
    battery_states_[power_hal::BatteryType::BATTERY_24V] = false;
    battery_states_[power_hal::BatteryType::BATTERY_72V] = false;
    
    simulated_voltages_[power_hal::BatteryType::BATTERY_12V] = 12.0f;
    simulated_voltages_[power_hal::BatteryType::BATTERY_24V] = 24.0f;
    simulated_voltages_[power_hal::BatteryType::BATTERY_72V] = 72.0f;
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
    
    control_sub_ = this->create_subscription<power_hal::msg::BatteryControl>(
      "/power_hal/battery_control", 10,
      std::bind(&PowerHalNode::on_control_callback, this, std::placeholders::_1));
    
    status_pub_ = this->create_publisher<power_hal::msg::BatteryStatus>(
      "/power_hal/battery_status", 10);
    
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
    control_sub_.reset();
    status_pub_.reset();
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
  void on_timer()
  {
    if (simulation_mode_) {
      publish_simulated_status();
    } else {
      power_hal::BatteryStatusData status;
      while (can_interface_.receive_status(status)) {
        battery_states_[status.battery_type] = status.switch_state;
        
        auto msg = power_hal::msg::BatteryStatus();
        msg.battery_type = static_cast<uint8_t>(status.battery_type);
        msg.switch_state = status.switch_state;
        msg.total_voltage = status.bms_data.total_voltage;
        msg.total_current = status.bms_data.total_current;
        msg.remain_capacity = status.bms_data.remain_capacity;
        msg.full_capacity = status.bms_data.full_capacity;
        msg.soc = status.bms_data.soc;
        msg.cycle_count = status.bms_data.cycle_count;
        msg.max_cell_vol = status.bms_data.max_cell_vol;
        msg.max_cell_id = status.bms_data.max_cell_id;
        msg.min_cell_vol = status.bms_data.min_cell_vol;
        msg.min_cell_id = status.bms_data.min_cell_id;
        msg.max_temp = status.bms_data.max_temp;
        msg.min_temp = status.bms_data.min_temp;
        msg.cell_count = status.bms_data.cell_count;
        msg.protection_status = status.bms_data.protection_status;
        msg.mos_charge_state = status.bms_data.mos_charge_state;
        msg.mos_discharge_state = status.bms_data.mos_discharge_state;
        
        for (int i = 0; i < 32; i++) {
          msg.cell_voltages[i] = status.bms_data.cell_voltages[i];
        }
        
        for (int i = 0; i < 8; i++) {
          msg.temperatures[i] = status.bms_data.temperatures[i];
        }
        
        msg.stamp = this->now();
        
        status_pub_->publish(msg);
        
        std::string color = msg.switch_state ? COLOR_GREEN : COLOR_RED;
        std::string state_str = msg.switch_state ? "开启" : "关闭";
        
        RCLCPP_INFO(this->get_logger(), 
            "%s【%s】%s%s %.2fV %.2fA SOC:%.1f%% 电量:%.2f/%.2fAh 循环:%d%s",
            color.c_str(),
            get_battery_name(msg.battery_type).c_str(),
            state_str.c_str(),
            COLOR_RESET,
            msg.total_voltage,
            msg.total_current,
            msg.soc,
            msg.remain_capacity,
            msg.full_capacity,
            msg.cycle_count,
            COLOR_RESET);
        
        if (msg.switch_state) {
            std::string cell_line = "  ";
            cell_line += COLOR_CYAN;
            cell_line += "单体:";
            for (int i = 0; i < msg.cell_count && i < 4; i++) {
                cell_line += " " + std::to_string(msg.cell_voltages[i] / 1000.0) + "V";
            }
            if (msg.cell_count > 4) cell_line += "...";
            cell_line += COLOR_RESET;
            RCLCPP_INFO(this->get_logger(), "%s", cell_line.c_str());
            
            std::string temp_line = "  ";
            temp_line += COLOR_YELLOW;
            temp_line += "温度:";
            int temp_count = 0;
            for (int i = 0; i < 8; i++) {
                if (msg.temperatures[i] != 25) {
                    temp_line += " " + std::to_string(msg.temperatures[i]) + "°C";
                    temp_count++;
                    if (temp_count >= 3) break;
                }
            }
            temp_line += COLOR_RESET;
            RCLCPP_INFO(this->get_logger(), "%s", temp_line.c_str());
        }
      }
    }
  }
  
  void publish_simulated_status()
  {
    static int counter = 0;
    counter++;
    
    for (const auto& [battery_type, switch_state] : battery_states_) {
      if (counter % 10 == 0) {
        auto msg = power_hal::msg::BatteryStatus();
        msg.battery_type = static_cast<uint8_t>(battery_type);
        msg.switch_state = switch_state;
        
        float nominal_voltage = get_nominal_voltage(battery_type);
        msg.total_voltage = switch_state ? nominal_voltage : 0.0f;
        msg.total_current = switch_state ? 5.0f : 0.0f;
        msg.remain_capacity = switch_state ? nominal_voltage * 10.0f : 0.0f;
        msg.full_capacity = nominal_voltage * 10.0f;
        msg.soc = switch_state ? 85.0f : 0.0f;
        msg.cycle_count = 100;
        msg.max_cell_vol = switch_state ? nominal_voltage / 4.0f : 0.0f;
        msg.max_cell_id = 0;
        msg.min_cell_vol = switch_state ? nominal_voltage / 4.0f : 0.0f;
        msg.min_cell_id = 0;
        msg.max_temp = switch_state ? 35 : 25;
        msg.min_temp = switch_state ? 30 : 20;
        msg.cell_count = 4;
        msg.protection_status = 0;
        msg.mos_charge_state = false;
        msg.mos_discharge_state = switch_state;
        
        for (int i = 0; i < 32; i++) {
          msg.cell_voltages[i] = switch_state ? (uint16_t)(nominal_voltage / 4.0f * 1000.0f) : 0;
        }
        
        for (int i = 0; i < 8; i++) {
          msg.temperatures[i] = switch_state ? 30 + (i % 5) : 25;
        }
        
        msg.stamp = this->now();
        
        status_pub_->publish(msg);
        
        std::string color = msg.switch_state ? COLOR_GREEN : COLOR_RED;
        std::string state_str = msg.switch_state ? "开启" : "关闭";
        
        RCLCPP_INFO(this->get_logger(), 
            "%s[模拟模式]【%s】%s%s %.2fV %.2fA SOC:%.1f%% 电量:%.2f/%.2fAh 循环:%d%s",
            color.c_str(),
            get_battery_name(msg.battery_type).c_str(),
            state_str.c_str(),
            COLOR_RESET,
            msg.total_voltage,
            msg.total_current,
            msg.soc,
            msg.remain_capacity,
            msg.full_capacity,
            msg.cycle_count,
            COLOR_RESET);
        
        if (msg.switch_state) {
            std::string cell_line = "  ";
            cell_line += COLOR_CYAN;
            cell_line += "单体:";
            for (int i = 0; i < msg.cell_count && i < 4; i++) {
                cell_line += " " + std::to_string(msg.cell_voltages[i] / 1000.0) + "V";
            }
            if (msg.cell_count > 4) cell_line += "...";
            cell_line += COLOR_RESET;
            RCLCPP_INFO(this->get_logger(), "%s", cell_line.c_str());
            
            std::string temp_line = "  ";
            temp_line += COLOR_YELLOW;
            temp_line += "温度:";
            int temp_count = 0;
            for (int i = 0; i < 8; i++) {
                if (msg.temperatures[i] != 25) {
                    temp_line += " " + std::to_string(msg.temperatures[i]) + "°C";
                    temp_count++;
                    if (temp_count >= 3) break;
                }
            }
            temp_line += COLOR_RESET;
            RCLCPP_INFO(this->get_logger(), "%s", temp_line.c_str());
        }
      }
    }
  }
  
  void on_control_callback(const power_hal::msg::BatteryControl::SharedPtr msg)
  {
    auto battery_type = static_cast<power_hal::BatteryType>(msg->battery_type);
    
    if (simulation_mode_) {
      battery_states_[battery_type] = msg->switch_state;
      if (msg->switch_state) {
        simulated_voltages_[battery_type] = get_nominal_voltage(battery_type);
      } else {
        simulated_voltages_[battery_type] = 0.0f;
      }
      RCLCPP_INFO(this->get_logger(), 
                   "[模拟模式] 控制指令执行: 【%s】-> %s",
                   get_battery_name(msg->battery_type).c_str(),
                   msg->switch_state ? "开启" : "关闭");
    } else {
      power_hal::BatteryControlData control;
      control.battery_type = battery_type;
      control.switch_state = msg->switch_state;
      
      if (can_interface_.send_control_command(control)) {
        battery_states_[battery_type] = msg->switch_state;
        RCLCPP_INFO(this->get_logger(), 
                   "控制指令执行: 【%s】-> %s",
                   get_battery_name(msg->battery_type).c_str(),
                   msg->switch_state ? "开启" : "关闭");
      } else {
        RCLCPP_ERROR(this->get_logger(), 
                     "发送【%s】控制指令失败",
                     get_battery_name(msg->battery_type).c_str());
      }
    }
  }
  
  float get_nominal_voltage(power_hal::BatteryType battery_type)
  {
    switch (battery_type) {
      case power_hal::BatteryType::BATTERY_12V: return 12.0f;
      case power_hal::BatteryType::BATTERY_24V: return 24.0f;
      case power_hal::BatteryType::BATTERY_72V: return 72.0f;
      default: return 0.0f;
    }
  }
  std::string get_battery_name(uint8_t battery_type) {
    switch (battery_type) {
      case 0: return "12V电池";
      case 1: return "24V电池";
      case 2: return "72V电池";
      default: return "未知电池";
    }
  }
  rclcpp::TimerBase::SharedPtr timer_;
  rclcpp::Subscription<power_hal::msg::BatteryControl>::SharedPtr control_sub_;
  rclcpp::Publisher<power_hal::msg::BatteryStatus>::SharedPtr status_pub_;
  power_hal::CanInterface can_interface_;
  std::map<power_hal::BatteryType, bool> battery_states_;
  std::map<power_hal::BatteryType, float> simulated_voltages_;
  bool simulation_mode_;
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  
  auto node = std::make_shared<PowerHalNode>("Power_Hal");
  
  rclcpp::spin(node->get_node_base_interface());
  
  rclcpp::shutdown();
  return 0;
}