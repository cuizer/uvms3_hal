#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>

#include "hal/msg/hal_acoustic.hpp"
#include "hal/msg/hal_battery.hpp"
#include "hal/msg/hal_depthsensor.hpp"
#include "hal/msg/hal_inertialnavi.hpp"

#include <algorithm>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

using CallbackReturn =
  rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

namespace
{

int setup_native_uart(const std::string & port_name, speed_t baud_rate)
{
  int fd = open(port_name.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
  if (fd == -1) {
    return -1;
  }

  struct termios tty;
  if (tcgetattr(fd, &tty) != 0) {
    close(fd);
    return -1;
  }

  cfmakeraw(&tty);
  cfsetospeed(&tty, baud_rate);
  cfsetispeed(&tty, baud_rate);
  tty.c_cflag |= CREAD | CLOCAL;
  tty.c_cflag &= ~PARENB;
  tty.c_cflag &= ~CSTOPB;
  tty.c_cflag &= ~CSIZE;
  tty.c_cflag |= CS8;
  tty.c_cflag &= ~CRTSCTS;
  tty.c_cc[VMIN] = 0;
  tty.c_cc[VTIME] = 1;

  if (tcsetattr(fd, TCSANOW, &tty) != 0) {
    close(fd);
    return -1;
  }

  tcflush(fd, TCIFLUSH);
  fcntl(fd, F_SETFL, O_NONBLOCK);
  return fd;
}

speed_t baud_to_termios(int baud_rate)
{
  switch (baud_rate) {
    case 9600:
      return B9600;
    case 19200:
      return B19200;
    case 38400:
      return B38400;
    case 57600:
      return B57600;
    case 115200:
      return B115200;
    default:
      return B19200;
  }
}

void append_u16_le(std::vector<uint8_t> & out, uint16_t value)
{
  out.push_back(static_cast<uint8_t>(value & 0xFF));
  out.push_back(static_cast<uint8_t>((value >> 8) & 0xFF));
}

void append_i16_le(std::vector<uint8_t> & out, int16_t value)
{
  append_u16_le(out, static_cast<uint16_t>(value));
}

void append_f32_le(std::vector<uint8_t> & out, float value)
{
  uint8_t bytes[sizeof(float)];
  std::memcpy(bytes, &value, sizeof(float));
  out.insert(out.end(), bytes, bytes + sizeof(float));
}

uint8_t xor8(const std::vector<uint8_t> & data, size_t begin)
{
  uint8_t checksum = 0;
  for (size_t i = begin; i < data.size(); ++i) {
    checksum ^= data[i];
  }
  return checksum;
}

}  // namespace

class HalAcousticNode : public rclcpp_lifecycle::LifecycleNode
{
public:
  explicit HalAcousticNode(const std::string & node_name)
  : rclcpp_lifecycle::LifecycleNode(node_name)
  {
    declare_parameter<std::string>("port_name", "/dev/ttyUSB0");
    declare_parameter<int>("baud_rate", 19200);
    declare_parameter<int>("target_id", 0x00);
  }

  CallbackReturn on_configure(const rclcpp_lifecycle::State &) override
  {
    auto qos = rclcpp::QoS(10);
    acoustic_sub_ = create_subscription<hal::msg::HalAcoustic>(
      "/hal/acoustic",
      qos,
      std::bind(&HalAcousticNode::acoustic_callback, this, std::placeholders::_1));
    inertial_sub_ = create_subscription<hal::msg::HalInertialnavi>(
      "/hal/inertialnavi",
      qos,
      std::bind(&HalAcousticNode::inertial_callback, this, std::placeholders::_1));
    depth_sub_ = create_subscription<hal::msg::HalDepthsensor>(
      "/hal/depthsensor",
      qos,
      std::bind(&HalAcousticNode::depth_callback, this, std::placeholders::_1));
    battery_sub_ = create_subscription<hal::msg::HalBattery>(
      "/hal/battery",
      qos,
      std::bind(&HalAcousticNode::battery_callback, this, std::placeholders::_1));

    RCLCPP_INFO(get_logger(), "水声通信节点已配置，等待激活。");
    return CallbackReturn::SUCCESS;
  }

  CallbackReturn on_activate(const rclcpp_lifecycle::State & state) override
  {
    const auto port = get_parameter("port_name").as_string();
    const int baud_rate = get_parameter("baud_rate").as_int();
    serial_fd_ = setup_native_uart(port, baud_to_termios(baud_rate));
    if (serial_fd_ < 0) {
      RCLCPP_ERROR(
        get_logger(),
        "水声通信串口 %s 打开失败: %s",
        port.c_str(),
        std::strerror(errno));
      return CallbackReturn::FAILURE;
    }

    active_ = true;
    RCLCPP_INFO(get_logger(), "水声通信节点已激活: %s @ %d", port.c_str(), baud_rate);
    return LifecycleNode::on_activate(state);
  }

  CallbackReturn on_deactivate(const rclcpp_lifecycle::State & state) override
  {
    active_ = false;
    close_serial();
    return LifecycleNode::on_deactivate(state);
  }

  CallbackReturn on_cleanup(const rclcpp_lifecycle::State &) override
  {
    active_ = false;
    close_serial();
    acoustic_sub_.reset();
    inertial_sub_.reset();
    depth_sub_.reset();
    battery_sub_.reset();
    return CallbackReturn::SUCCESS;
  }

  CallbackReturn on_shutdown(const rclcpp_lifecycle::State &) override
  {
    active_ = false;
    close_serial();
    return CallbackReturn::SUCCESS;
  }

private:
  rclcpp::Subscription<hal::msg::HalAcoustic>::SharedPtr acoustic_sub_;
  rclcpp::Subscription<hal::msg::HalInertialnavi>::SharedPtr inertial_sub_;
  rclcpp::Subscription<hal::msg::HalDepthsensor>::SharedPtr depth_sub_;
  rclcpp::Subscription<hal::msg::HalBattery>::SharedPtr battery_sub_;

  std::mutex data_mutex_;
  std::optional<hal::msg::HalInertialnavi> latest_inertial_;
  std::optional<hal::msg::HalDepthsensor> latest_depth_;
  std::optional<hal::msg::HalBattery> latest_battery_;

  std::mutex serial_mutex_;
  int serial_fd_{-1};
  bool active_{false};
  uint16_t sequence_{0};

  void inertial_callback(const hal::msg::HalInertialnavi::SharedPtr msg)
  {
    std::lock_guard<std::mutex> lock(data_mutex_);
    latest_inertial_ = *msg;
  }

  void depth_callback(const hal::msg::HalDepthsensor::SharedPtr msg)
  {
    std::lock_guard<std::mutex> lock(data_mutex_);
    latest_depth_ = *msg;
  }

  void battery_callback(const hal::msg::HalBattery::SharedPtr msg)
  {
    std::lock_guard<std::mutex> lock(data_mutex_);
    latest_battery_ = *msg;
  }

  void acoustic_callback(const hal::msg::HalAcoustic::SharedPtr msg)
  {
    if (!active_) {
      return;
    }

    if (msg->command == 0) {
      RCLCPP_DEBUG(get_logger(), "收到水声发送控制 0，不发送。");
      return;
    }
    if (msg->command != 1) {
      RCLCPP_WARN(get_logger(), "水声发送控制非法: %u，有效值 0/1", msg->command);
      return;
    }

    std::optional<hal::msg::HalInertialnavi> inertial;
    std::optional<hal::msg::HalDepthsensor> depth;
    std::optional<hal::msg::HalBattery> battery;
    {
      std::lock_guard<std::mutex> lock(data_mutex_);
      inertial = latest_inertial_;
      depth = latest_depth_;
      battery = latest_battery_;
    }

    if (!inertial || !depth || !battery) {
      RCLCPP_WARN(
        get_logger(),
        "水声发送被跳过: inertial=%d depth=%d battery=%d",
        inertial.has_value(),
        depth.has_value(),
        battery.has_value());
      return;
    }

    const auto payload = build_status_payload(*inertial, *depth, *battery);
    const auto frame = build_acoustic_frame(payload);
    write_frame(frame);
  }

  std::vector<uint8_t> build_status_payload(
    const hal::msg::HalInertialnavi & inertial,
    const hal::msg::HalDepthsensor & depth,
    const hal::msg::HalBattery & battery)
  {
    std::vector<uint8_t> payload;
    payload.reserve(43);

    payload.push_back(0x55);
    payload.push_back(0x01);
    append_u16_le(payload, sequence_++);

    uint8_t valid_mask = 0;
    if (inertial.connection_status == 1) {
      valid_mask |= 0x01;
    }
    if (depth.connection_status == 1) {
      valid_mask |= 0x02;
    }
    valid_mask |= 0x04;
    payload.push_back(valid_mask);

    append_f32_le(payload, inertial.yaw);
    append_f32_le(payload, inertial.pitch);
    append_f32_le(payload, inertial.roll);
    append_f32_le(payload, inertial.latitude);
    append_f32_le(payload, inertial.longitude);
    append_f32_le(payload, depth.depth_avg);

    payload.push_back(static_cast<uint8_t>(battery.battery_status_48v));
    payload.push_back(static_cast<uint8_t>(battery.battery_status_72v));
    append_u16_le(payload, battery.battery_voltage_48v);
    append_u16_le(payload, battery.battery_voltage_72v);
    append_i16_le(payload, battery.battery_current_48v);
    append_i16_le(payload, battery.battery_current_72v);
    append_u16_le(payload, battery.battery_temperature_48v);
    append_u16_le(payload, battery.battery_temperature_72v);

    return payload;
  }

  std::vector<uint8_t> build_acoustic_frame(const std::vector<uint8_t> & payload)
  {
    constexpr uint8_t kSendNoAckCommand = 0x11;
    const int target_id_param = get_parameter("target_id").as_int();
    const uint8_t target_id =
      static_cast<uint8_t>(std::clamp(target_id_param, 0x00, 0xFE));
    const uint16_t payload_len = static_cast<uint16_t>(payload.size());

    std::vector<uint8_t> frame;
    frame.reserve(2 + 1 + 1 + 2 + payload.size() + 1 + 2);
    frame.push_back(0x24);
    frame.push_back(0x24);
    frame.push_back(kSendNoAckCommand);
    frame.push_back(target_id);
    frame.push_back(static_cast<uint8_t>((payload_len >> 8) & 0xFF));
    frame.push_back(static_cast<uint8_t>(payload_len & 0xFF));
    frame.insert(frame.end(), payload.begin(), payload.end());
    frame.push_back(xor8(frame, 2));
    frame.push_back(0x40);
    frame.push_back(0x40);
    return frame;
  }

  void write_frame(const std::vector<uint8_t> & frame)
  {
    std::lock_guard<std::mutex> lock(serial_mutex_);
    if (serial_fd_ < 0) {
      RCLCPP_WARN(get_logger(), "水声通信串口未打开，无法发送。");
      return;
    }

    const ssize_t written = write(serial_fd_, frame.data(), frame.size());
    if (written != static_cast<ssize_t>(frame.size())) {
      RCLCPP_WARN(
        get_logger(),
        "水声通信发送失败: written=%zd expected=%zu errno=%d",
        written,
        frame.size(),
        errno);
      return;
    }

    RCLCPP_INFO(get_logger(), "水声广播状态已发送: %zu bytes", frame.size());
  }

  void close_serial()
  {
    std::lock_guard<std::mutex> lock(serial_mutex_);
    if (serial_fd_ >= 0) {
      close(serial_fd_);
      serial_fd_ = -1;
    }
  }
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = std::make_shared<HalAcousticNode>("hal_acoustic_node");
  rclcpp::executors::SingleThreadedExecutor exec;
  exec.add_node(node->get_node_base_interface());
  exec.spin();
  rclcpp::shutdown();
  return 0;
}
