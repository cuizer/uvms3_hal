#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include <rclcpp_lifecycle/lifecycle_publisher.hpp>
#include "hal/msg/hal_dvl_msg.hpp" 
#include <thread>
#include <mutex>
#include <chrono>
#include <string>
#include <sstream> 
#include <vector>

// 引入 Linux 底层串口所需的系统头文件
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <sys/ioctl.h> // 【新增】用于查询串口缓冲区是否有数据

/**
 * @brief 原生 Linux 串口初始化函数 
 * @return 成功返回文件描述符 (fd > 0)，失败返回 -1
 */
int setup_native_uart(const std::string& port_name) {
    int fd = open(port_name.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) return -1;

    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        close(fd);
        return -1;
    }

    cfsetospeed(&tty, B115200);
    cfsetispeed(&tty, B115200);

    tty.c_cflag &= ~CSIZE;   
    tty.c_cflag |= CS8;      
    tty.c_cflag &= ~PARENB;  
    tty.c_cflag &= ~CSTOPB;  

    tty.c_cflag &= ~CRTSCTS;                
    tty.c_iflag &= ~(IXON | IXOFF | IXANY); 

    tty.c_cflag |= CREAD | CLOCAL;

    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_oflag &= ~OPOST;

    tty.c_cc[VMIN]  = 0;
    tty.c_cc[VTIME] = 1;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        close(fd);
        return -1;
    }

    tcflush(fd, TCIFLUSH);
    fcntl(fd, F_SETFL, 0);

    return fd;
}

using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;
using namespace std::chrono_literals;

class HalDvlNode : public rclcpp_lifecycle::LifecycleNode
{
public:
    HalDvlNode(const std::string & node_name, bool intra_process_comms = false)
    : rclcpp_lifecycle::LifecycleNode(node_name, rclcpp::NodeOptions().use_intra_process_comms(intra_process_comms))
    {
        this->declare_parameter<std::string>("port_name", "/dev/ttyTHS2");
    }

    CallbackReturn on_configure(const rclcpp_lifecycle::State &) override {
        dvl_pub_ = this->create_publisher<hal::msg::HalDvlMsg>("hal_dvl_msg", 10);
        
        publish_timer_ = this->create_wall_timer(
            20ms, std::bind(&HalDvlNode::publish_timer_callback, this));
            
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_activate(const rclcpp_lifecycle::State & state) override {
        dvl_pub_->on_activate();
        is_running_ = true;
        dvl_thread_ = std::thread(&HalDvlNode::dvl_thread_function, this);
        return LifecycleNode::on_activate(state);
    }

    CallbackReturn on_deactivate(const rclcpp_lifecycle::State & state) override {
        dvl_pub_->on_deactivate();
        is_running_ = false;
        if (dvl_thread_.joinable()) dvl_thread_.join();
        
        // 【修复】语法错误并替换为原生关闭串口
        if (serial_fd_ >= 0) {
            close(serial_fd_);
            serial_fd_ = -1;
        }
        return LifecycleNode::on_deactivate(state);
    }

    CallbackReturn on_cleanup(const rclcpp_lifecycle::State &) override {
        dvl_pub_.reset();
        publish_timer_.reset();
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_shutdown(const rclcpp_lifecycle::State &) override {
        is_running_ = false;
        if (dvl_thread_.joinable()) dvl_thread_.join();
        
        // 【替换】原生关闭串口
        if (serial_fd_ >= 0) {
            close(serial_fd_);
            serial_fd_ = -1;
        }
        return CallbackReturn::SUCCESS;
    }

private:
    std::shared_ptr<rclcpp_lifecycle::LifecyclePublisher<hal::msg::HalDvlMsg>> dvl_pub_;
    rclcpp::TimerBase::SharedPtr publish_timer_; 
    
    hal::msg::HalDvlMsg cached_msg_; 
    std::mutex msg_mutex_;           

    int serial_fd_ = -1; // 仅保留这一个原生文件描述符
    std::thread dvl_thread_;
    std::atomic<bool> is_running_{false};

    void publish_timer_callback() {
        if (dvl_pub_->is_activated()) {
            hal::msg::HalDvlMsg msg_to_publish;
            {
                std::lock_guard<std::mutex> lock(msg_mutex_);
                msg_to_publish = cached_msg_;
            }
            dvl_pub_->publish(msg_to_publish);
        }
    }

   void parse_and_cache(const std::string& data, int64_t capture_time_ns) {
        std::vector<std::string> tokens;
        std::stringstream ss(data);
        std::string item;
        
        // 按照逗号拆分报文
        while (std::getline(ss, item, ',')) {
            tokens.push_back(item);
        }

        // 假设你们的 DVL 报文有至少 5 个字段 (例如: wrz,vx,vy,vz,valid)
        if (tokens.size() >= 5) {
            try {
                // 使用 .empty() 检查，防止空字符串导致 std::stof 抛出异常
                float vx = tokens[1].empty() ? 0.0f : std::stof(tokens[1]);
                float vy = tokens[2].empty() ? 0.0f : std::stof(tokens[2]);
                float vz = tokens[3].empty() ? 0.0f : std::stof(tokens[3]);
                
                std::string valid = tokens[4];
                // 剔除末尾可能带有的 \r \n 空格等不可见字符
                valid.erase(valid.find_last_not_of(" \n\r\t") + 1);

                // 如果底面锁定且数据有效 (根据实际 DVL 说明书可能为 'y' 或其他标识)
                if (valid == "y" || valid == "Y") {
                    std::lock_guard<std::mutex> lock(msg_mutex_);
                    cached_msg_.timestamp = capture_time_ns;
                    cached_msg_.velocity_x = vx;
                    cached_msg_.velocity_y = vy;
                    cached_msg_.velocity_z = vz;
                } else {
                    RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 2000, 
                                         "DVL未锁定底部，当前数据丢弃!");
                }
            } catch (const std::exception& e) {
                // 捕获到乱码导致转换失败，只打印 Debug 警告，绝不断开原生串口！
                RCLCPP_DEBUG(this->get_logger(), "DVL 脏数据解析跳过 (乱码): %s", e.what());
            }
        }
    }

    void dvl_thread_function() {
        std::string buffer = "";
        char read_buf[1024]; // 用于原生 read 的临时数组

        while (rclcpp::ok() && is_running_) {
            try {
                // 1. 如果串口未打开，则尝试打开 (替代 !serial_.isOpen())
                if (serial_fd_ < 0) {
                    std::string port = this->get_parameter("port_name").as_string();
                    serial_fd_ = setup_native_uart(port);
                    if (serial_fd_ < 0) {
                        std::this_thread::sleep_for(1s); // 失败则等1秒再试
                        continue;
                    }
                    RCLCPP_INFO(this->get_logger(), "DVL 原生串口已打开: %s", port.c_str());
                }

                // 2. 查询缓冲区里有多少字节 (替代 serial_.available())
                int available_bytes = 0;
                if (ioctl(serial_fd_, FIONREAD, &available_bytes) < 0) {
                    throw std::runtime_error("底层 ioctl 通信错误，串口可能已断开");
                }

                if (available_bytes > 0) {
                    // 第一时间抓取时间戳
                    int64_t capture_time_ns = this->now().nanoseconds();
                    
                    // 限制最大读取量防止数组越界
                    int bytes_to_read = std::min(available_bytes, (int)sizeof(read_buf));
                    
                    // 3. 原生读取 (替代 serial_.read())
                    int bytes_read = read(serial_fd_, read_buf, bytes_to_read);
                    if (bytes_read < 0) {
                        throw std::runtime_error("原生 read 错误");
                    }

                    // 将读取到的 C 风格字符数组追加到 std::string 缓冲区中
                    buffer.append(read_buf, bytes_read);
                    
                    size_t pos = 0;
                    while ((pos = buffer.find("\r\n")) != std::string::npos) {
                        std::string line = buffer.substr(0, pos);
                        buffer.erase(0, pos + 2);
                        
                        if (line.rfind("$DVLHDR", 0) == 0) {
                            parse_and_cache(line, capture_time_ns); 
                        }
                    }
                    if (buffer.size() > 4096) buffer.clear(); 
                }
            } 
            // 4. 异常处理 (原生 API 报错会通过 throw 抛出到这里)
            catch (const std::exception& e) {
                RCLCPP_ERROR(this->get_logger(), "DVL 串口异常: %s", e.what());
                if (serial_fd_ >= 0) {
                    close(serial_fd_);
                    serial_fd_ = -1; // 触发下一次循环的重新打开
                }
                std::this_thread::sleep_for(2s); 
            }
            std::this_thread::sleep_for(10ms); 
        }
    }
};

int main(int argc, char ** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<HalDvlNode>("hal_dvl_node")->get_node_base_interface());
    rclcpp::shutdown();
    return 0;
}