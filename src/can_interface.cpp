#include "power_hal/can_interface.hpp"
#include <stdexcept>

namespace power_hal
{

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

}