#pragma once

#include <string>

#include "uvms_hal_manipulator/protocol_parser.hpp"

namespace uvms_hal_manipulator
{

class CanDriver
{
public:
    CanDriver();
    ~CanDriver();

    // 禁止拷贝
    CanDriver(const CanDriver&) = delete;
    CanDriver& operator=(const CanDriver&) = delete;

    // 打开/关闭 CAN 接口
    bool open(const std::string& if_name);
    void close();
    bool is_open() const;

    // 发送/接收单帧 CAN 报文
    bool write_frame(const CanFrame& frame);
    bool read_frame(CanFrame& frame);

    // 清空缓冲区
    void flush();

private:
    int socket_fd_;
    std::string if_name_;
};

}  // namespace uvms_hal_manipulator