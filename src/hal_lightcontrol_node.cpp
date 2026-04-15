#include <memory>
#include <chrono>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "std_msgs/msg/u_int8.hpp"

using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

class HalLightControlNode : public rclcpp_lifecycle::LifecycleNode {
public:
  explicit HalLightControlNode(const std::string & n) : rclcpp_lifecycle::LifecycleNode(n) {
    curr_ = 0; 
    targ_ = 0; 
    socket_fd_ = -1;
  }

  ~HalLightControlNode() {
    if (socket_fd_ != -1) close(socket_fd_);
  }

  CallbackReturn on_configure(const rclcpp_lifecycle::State &) override {
    // ---------------------------------------------------------
    // 1. 初始化 SocketCAN
    // ---------------------------------------------------------
    socket_fd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socket_fd_ < 0) {
        RCLCPP_ERROR(get_logger(), "SocketCAN 创建失败！");
        return CallbackReturn::FAILURE;
    }

    // 指定我们要绑定的 CAN 接口名字（例如 can0）
    struct ifreq ifr;
    strcpy(ifr.ifr_name, "can0");
    if (ioctl(socket_fd_, SIOCGIFINDEX, &ifr) < 0) {
        RCLCPP_ERROR(get_logger(), "找不到 can0 接口！请确认硬件是否连接，或使用 ifconfig -a 查看接口名");
        close(socket_fd_);
        socket_fd_ = -1;
        return CallbackReturn::FAILURE;
    }

    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(socket_fd_, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        RCLCPP_ERROR(get_logger(), "绑定 can0 接口失败！");
        close(socket_fd_);
        socket_fd_ = -1;
        return CallbackReturn::FAILURE;
    }

    RCLCPP_INFO(get_logger(), ">>> 物理 CAN 接口 can0 已就绪 <<<");

    // ---------------------------------------------------------
    // 2. 配置 ROS 2 通讯与平滑渐变逻辑
    // ---------------------------------------------------------
    sub_ = create_subscription<std_msgs::msg::UInt8>("hal_lightcontrol_srv", 10, 
      [this](std_msgs::msg::UInt8::SharedPtr msg){ targ_ = msg->data; });
      
    tmr_ = create_wall_timer(std::chrono::milliseconds(50), [this](){
      if (get_current_state().id() != 3 || curr_ == targ_) return;
      
      // 平滑计算逻辑
      curr_ += (targ_ > curr_) ? std::min((uint8_t)5, (uint8_t)(targ_-curr_)) : -std::min((uint8_t)5, (uint8_t)(curr_-targ_));
      
      // ---------------------------------------------------------
      // 3. 将真实数据写入 CAN 总线
      // ---------------------------------------------------------
      if (socket_fd_ != -1) {
          struct can_frame frame;
          frame.can_id = 0x123;    // TODO: 替换为你实际需要的 CAN ID
          frame.can_dlc = 8;       // 数据长度 (Data Length Code)，最大 8 字节
          
          // 初始化数据段为 0
          memset(frame.data, 0, sizeof(frame.data)); 
          
          // 假设我们将亮度值放在第 0 个字节
          frame.data[0] = curr_;   
          
          // 发送 CAN 帧
          if (write(socket_fd_, &frame, sizeof(struct can_frame)) != sizeof(struct can_frame)) {
              RCLCPP_ERROR(get_logger(), "CAN 数据发送失败！");
          } else {
              RCLCPP_INFO(get_logger(), "发送 CAN [ID: 0x123] 亮度: %u", curr_);
          }
      }
    });
    
    return CallbackReturn::SUCCESS;
  }

  CallbackReturn on_activate(const rclcpp_lifecycle::State & s) override {
    rclcpp_lifecycle::LifecycleNode::on_activate(s);
    RCLCPP_INFO(get_logger(), "灯光节点已激活，准备接收指令");
    return CallbackReturn::SUCCESS;
  }

  CallbackReturn on_cleanup(const rclcpp_lifecycle::State & s) override {
    if (socket_fd_ != -1) {
        close(socket_fd_);
        socket_fd_ = -1;
    }
    rclcpp_lifecycle::LifecycleNode::on_cleanup(s);
    return CallbackReturn::SUCCESS;
  }

private:
  rclcpp::Subscription<std_msgs::msg::UInt8>::SharedPtr sub_;
  rclcpp::TimerBase::SharedPtr tmr_;
  uint8_t curr_, targ_;
  int socket_fd_; 
};

int main(int argc, char ** argv) {
  rclcpp::init(argc, argv);
  auto node = std::make_shared<HalLightControlNode>("hal_lightcontrol_node");
  rclcpp::spin(node->get_node_base_interface());
  rclcpp::shutdown();
  return 0;
}
