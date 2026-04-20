#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include "hal/msg/hal_dvl_msg.hpp" 
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <string>
#include <sstream> 
#include <atomic>

// 引入 Linux 底层串口所需的系统头文件
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <sys/select.h> 

/**
 * @brief 原生 Linux 串口初始化函数 
 * @param port_name 串口设备路径
 * @param baud_rate 波特率宏定义 (如 B115200)
 */
int setup_native_uart(const std::string& port_name, speed_t baud_rate) {
    int fd = open(port_name.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd == -1) return -1;

    struct termios tty;
    if (tcgetattr(fd, &tty) != 0) {
        close(fd);
        return -1;
    }

    cfsetospeed(&tty, baud_rate);
    cfsetispeed(&tty, baud_rate);

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
    fcntl(fd, F_SETFL, O_NONBLOCK);

    return fd;
}

using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;
using namespace std::chrono_literals;

class HalDvlNode : public rclcpp_lifecycle::LifecycleNode
{
public:
    HalDvlNode(const std::string & node_name)
    : rclcpp_lifecycle::LifecycleNode(node_name)
    {
        // 声明参数：端口和波特率
        this->declare_parameter<std::string>("port_name", "/dev/ttyUSB0");
        this->declare_parameter<int>("baud_rate", 115200);
    }

    CallbackReturn on_configure(const rclcpp_lifecycle::State &) override {
        dvl_pub_ = this->create_publisher<hal::msg::HalDvlMsg>("hal_dvl_msg", 10);
        
        // 创建 50Hz 的定时发布器
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
        // 假设报文格式为: $DVLHDR,y,1.25,-0.15,0.05*Cs
        size_t star_pos = data.find('*');
        std::string data_body = (star_pos != std::string::npos) ? data.substr(0, star_pos) : data;
        
        std::vector<std::string> tokens;
        std::stringstream ss(data_body);
        std::string token;
        
        while (std::getline(ss, token, ',')) {
            tokens.push_back(token);
        }

        // 确保字段数足够（至少包含头、状态、vx, vy, vz）
        if (tokens.size() >= 5) {
            std::lock_guard<std::mutex> lock(msg_mutex_);
            cached_msg_.timestamp = capture_time_ns;

            // 检查失锁状态（tokens[1] 为状态位 'y' 表示有效，'n' 表示失锁）
            if (tokens[1] == "y" || tokens[1] == "Y") {
                try {
                    cached_msg_.velocity_x = std::stof(tokens[2]);
                    cached_msg_.velocity_y = std::stof(tokens[3]);
                    cached_msg_.velocity_z = std::stof(tokens[4]);
                } catch (const std::exception& e) {
                    RCLCPP_DEBUG(this->get_logger(), "DVL 速度转换异常: %s", e.what());
                }
            } else {
                // 失锁保护：强制速度归零
                RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 2000, 
                    "DVL 底面失锁！已强制速度归零以保护系统");
                cached_msg_.velocity_x = 0.0f;
                cached_msg_.velocity_y = 0.0f;
                cached_msg_.velocity_z = 0.0f;
            }
        }
    }

    void dvl_thread_function() {
        std::string buffer = "";
        char read_buf[512]; 

        while (rclcpp::ok() && is_running_) {
            try {
                if (serial_fd_ < 0) {
                    std::string port = this->get_parameter("port_name").as_string();
                    int baud_int = this->get_parameter("baud_rate").as_int();
                    speed_t baud_rate = (baud_int == 115200) ? B115200 : 
                                        (baud_int == 460800) ? B460800 : B9600; // 简化的波特率映射

                    serial_fd_ = setup_native_uart(port, baud_rate);
                    if (serial_fd_ < 0) {
                        std::this_thread::sleep_for(1s);
                        continue;
                    }
                    RCLCPP_INFO(this->get_logger(), "DVL 原生串口已打开: %s", port.c_str());
                }

                fd_set read_fds;
                FD_ZERO(&read_fds);
                FD_SET(serial_fd_, &read_fds);

                struct timeval tv;
                tv.tv_sec = 0;
                tv.tv_usec = 5000; // 5ms 超时

                int ret = select(serial_fd_ + 1, &read_fds, NULL, NULL, &tv);

                if (ret < 0) {
                    throw std::runtime_error("select 监听底层错误");
                } 
                else if (ret > 0 && FD_ISSET(serial_fd_, &read_fds)) {
                    int bytes_read = read(serial_fd_, read_buf, sizeof(read_buf));
                    
                    if (bytes_read < 0) {
                        if (errno != EAGAIN && errno != EWOULDBLOCK) {
                            throw std::runtime_error("原生 read 失败");
                        }
                    } else if (bytes_read > 0) {
                        int64_t capture_time_ns = this->now().nanoseconds();
                        buffer.append(read_buf, bytes_read);
                        
                        size_t pos = 0;
                        size_t processed_pos = 0;
                        
                        while ((pos = buffer.find("\r\n", processed_pos)) != std::string::npos) {
                            std::string line = buffer.substr(processed_pos, pos - processed_pos);
                            processed_pos = pos + 2; 
                            
                            // 注意：如果您直连 WaterLinked DVL，这里的协议头可能需要改成 ":BI"
                            if (line.rfind("$DVLHDR", 0) == 0) {
                                parse_and_cache(line, capture_time_ns); 
                            }
                        }
                        
                        if (processed_pos > 0) {
                            buffer.erase(0, processed_pos);
                        }
                        if (buffer.size() > 4096) buffer.clear(); 
                    }
                }
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