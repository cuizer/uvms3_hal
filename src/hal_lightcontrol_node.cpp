#include <memory>
#include <chrono>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "std_msgs/msg/u_int8.hpp"

using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

class HalLightControlNode : public rclcpp_lifecycle::LifecycleNode {
public:
  explicit HalLightControlNode(const std::string & n) : rclcpp_lifecycle::LifecycleNode(n) {
    curr_ = 0; targ_ = 0;
  }

  CallbackReturn on_configure(const rclcpp_lifecycle::State &) override {
    sub_ = create_subscription<std_msgs::msg::UInt8>("hal_lightcontrol_srv", 10, 
      [this](std_msgs::msg::UInt8::SharedPtr msg){ targ_ = msg->data; });
    tmr_ = create_wall_timer(std::chrono::milliseconds(50), [this](){
      if (get_current_state().id() != 3 || curr_ == targ_) return;
      curr_ += (targ_ > curr_) ? std::min(5, targ_-curr_) : -std::min(5, curr_-targ_);
      RCLCPP_INFO(get_logger(), "亮度: %u", curr_);
    });
    return CallbackReturn::SUCCESS;
  }

  CallbackReturn on_activate(const rclcpp_lifecycle::State & s) override {
    rclcpp_lifecycle::LifecycleNode::on_activate(s);
    RCLCPP_INFO(get_logger(), "灯光节点已激活");
    return CallbackReturn::SUCCESS;
  }

private:
  rclcpp::Subscription<std_msgs::msg::UInt8>::SharedPtr sub_;
  rclcpp::TimerBase::SharedPtr tmr_;
  uint8_t curr_, targ_;
};

int main(int argc, char ** argv) {
  rclcpp::init(argc, argv);
  auto node = std::make_shared<HalLightControlNode>("hal_lightcontrol_node");
  rclcpp::spin(node->get_node_base_interface());
  rclcpp::shutdown();
  return 0;
}