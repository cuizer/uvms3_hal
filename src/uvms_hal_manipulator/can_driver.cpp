#include "uvms_hal_manipulator/can_driver.hpp"
#include <cstring>
#include <iostream>
#include <errno.h>
#include <fcntl.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <unistd.h>

namespace uvms_hal_manipulator
{

CanDriver::CanDriver()
: socket_fd_(-1)
{
}

CanDriver::~CanDriver()
{
    close();
}

bool CanDriver::open(const std::string& if_name)
{
    close();

    if_name_ = if_name;
    socket_fd_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (socket_fd_ < 0) {
        std::cerr << "[CanDriver] Failed to create CAN socket." << std::endl;
        return false;
    }

    struct ifreq ifr {};
    std::strncpy(ifr.ifr_name, if_name.c_str(), IFNAMSIZ - 1);

    if (ioctl(socket_fd_, SIOCGIFINDEX, &ifr) < 0) {
        std::cerr << "[CanDriver] Failed to get interface index for " << if_name << std::endl;
        close();
        return false;
    }

    struct sockaddr_can addr {};
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;

    if (bind(socket_fd_, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) < 0) {
        std::cerr << "[CanDriver] Failed to bind CAN socket to " << if_name << std::endl;
        close();
        return false;
    }

    int flags = fcntl(socket_fd_, F_GETFL, 0);
    if (flags < 0) {
        std::cerr << "[CanDriver] Failed to get socket flags." << std::endl;
        close();
        return false;
    }

    if (fcntl(socket_fd_, F_SETFL, flags | O_NONBLOCK) < 0) {
        std::cerr << "[CanDriver] Failed to set CAN socket non-blocking." << std::endl;
        close();
        return false;
    }

    return true;
}

void CanDriver::close()
{
    if (socket_fd_ >= 0) {
        ::close(socket_fd_);
        socket_fd_ = -1;
    }
}

bool CanDriver::is_open() const
{
    return socket_fd_ >= 0;
}

bool CanDriver::write_frame(const CanFrame& frame)
{
    if (!is_open()) {
        return false;
    }

    struct can_frame raw_frame {};
    raw_frame.can_id = frame.can_id;
    raw_frame.can_dlc = frame.dlc;

    for (uint8_t i = 0; i < frame.dlc && i < 8; ++i) {
        raw_frame.data[i] = frame.data[i];
    }

    const ssize_t nbytes = ::write(socket_fd_, &raw_frame, sizeof(raw_frame));
    if (nbytes != static_cast<ssize_t>(sizeof(raw_frame))) {
        std::cerr << "[CanDriver] Failed to write CAN frame." << std::endl;
        return false;
    }

    return true;
}

bool CanDriver::read_frame(CanFrame& frame)
{
    if (!is_open()) {
        return false;
    }

    struct can_frame raw_frame {};
    const ssize_t nbytes = ::read(socket_fd_, &raw_frame, sizeof(raw_frame));

    if (nbytes < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return false;
        }
        std::cerr << "[CanDriver] Failed to read CAN frame: "
                  << std::strerror(errno) << std::endl;
        return false;
    }

    if (nbytes == 0) {
        return false;
    }

    if (nbytes < static_cast<ssize_t>(sizeof(struct can_frame))) {
        std::cerr << "[CanDriver] Incomplete CAN frame received." << std::endl;
        return false;
    }

    frame.can_id = raw_frame.can_id & CAN_EFF_MASK;
    frame.dlc = raw_frame.can_dlc;
    frame.data.fill(0);

    for (uint8_t i = 0; i < frame.dlc && i < 8; ++i) {
        frame.data[i] = raw_frame.data[i];
    }

    return true;
}

void CanDriver::flush()
{
    if (!is_open()) {
        return;
    }

    CanFrame frame;
    while (read_frame(frame)) {}
}

}  // namespace uvms_hal_manipulator