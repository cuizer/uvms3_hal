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
#include <cstring>  // <--- 新增这一行！
// 引入 Linux 底层串口所需的系统头文件
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <sys/select.h> // 【新增】用于 select 机制

/**
 * @brief 原生 Linux 串口初始化函数 
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
    fcntl(fd, F_SETFL, O_NONBLOCK); // 确保非阻塞模式

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

    int serial_fd_ = -1; 
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
        
        while (std::getline(ss, item, ',')) {
            tokens.push_back(item);
        }

        if (tokens.size() >= 5) {
            try {
                float vx = tokens[1].empty() ? 0.0f : std::stof(tokens[1]);
                float vy = tokens[2].empty() ? 0.0f : std::stof(tokens[2]);
                float vz = tokens[3].empty() ? 0.0f : std::stof(tokens[3]);
                
                std::string valid = tokens[4];
                valid.erase(valid.find_last_not_of(" \n\r\t") + 1);

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
                RCLCPP_DEBUG(this->get_logger(), "DVL 脏数据解析跳过 (乱码): %s", e.what());
            }
        }
    }

    void dvl_thread_function() {
        std::string buffer = "";
        char read_buf[1024]; 

        while (rclcpp::ok() && is_running_) {
            try {
                if (serial_fd_ < 0) {
                    std::string port = this->get_parameter("port_name").as_string();
                    serial_fd_ = setup_native_uart(port);
                    if (serial_fd_ < 0) {
                        std::this_thread::sleep_for(1s); 
                        continue;
                    }
                    RCLCPP_INFO(this->get_logger(), "DVL 原生串口已打开: %s", port.c_str());
                }

                // 【优化 3】使用 select 取代 ioctl 和 sleep_for，实现数据秒唤醒
                fd_set read_fds;
                FD_ZERO(&read_fds);
                FD_SET(serial_fd_, &read_fds);

                struct timeval tv;
                tv.tv_sec = 0;
                tv.tv_usec = 10000; // 10ms 超时

                int ret = select(serial_fd_ + 1, &read_fds, NULL, NULL, &tv);
                
                if (ret < 0) {
                    throw std::runtime_error("select 监听底层错误");
                } 
                else if (ret > 0 && FD_ISSET(serial_fd_, &read_fds)) {
                    // 数据已就绪，直接读取
                    int bytes_read = read(serial_fd_, read_buf, sizeof(read_buf));
                    if (bytes_read < 0) {
                        if (errno != EAGAIN && errno != EWOULDBLOCK) {
                            throw std::runtime_error("原生 read 错误");
                        }
                    } else if (bytes_read > 0) {
                        int64_t capture_time_ns = this->now().nanoseconds();
                        buffer.append(read_buf, bytes_read);
                        
                        // 【优化 4】批量清理内存，消除频繁 erase 带来的 CPU 浪费和内存碎片化
                        size_t pos = 0;
                        size_t processed_pos = 0;
                        
                        while ((pos = buffer.find("\r\n", processed_pos)) != std::string::npos) {
                            std::string line = buffer.substr(processed_pos, pos - processed_pos);
                            processed_pos = pos + 2; // 只移动指针
                            
                            if (line.rfind("$DVLHDR", 0) == 0) {
                                parse_and_cache(line, capture_time_ns); 
                            }
                        }
                        
                        // 循环结束后一次性截断废弃内存
                        if (processed_pos > 0) {
                            buffer.erase(0, processed_pos);
                        }
                        
                        if (buffer.size() > 4096) buffer.clear(); // 防止脏数据无限累积
                    }
                }
                // 注意：去除了原来的 sleep_for(10ms)，因为 select 在没有数据时会自动挂起线程
            } 
            catch (const std::exception& e) {
                RCLCPP_ERROR(this->get_logger(), "DVL 串口异常: %s", e.what());
                if (serial_fd_ >= 0) {
                    close(serial_fd_);
                    serial_fd_ = -1; 
                }
                std::this_thread::sleep_for(2s); 
            }
        }
    }
};

int main(int argc, char ** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<HalDvlNode>("hal_dvl_node")->get_node_base_interface());
    rclcpp::shutdown();
    return 0;
}