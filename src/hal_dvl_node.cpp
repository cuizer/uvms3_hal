#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include "hal/msg/hal_dvl.hpp" 
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

    // 【关键修改 1】强制将串口设为完全透传模式 (Raw Mode)
    // 这一步会关闭底层的 ICRNL、ECHO、ICANON 等所有自动转换机制，确保读到的数据原汁原味
    cfmakeraw(&tty);

    cfsetospeed(&tty, baud_rate);
    cfsetispeed(&tty, baud_rate);

    tty.c_cflag |= CREAD | CLOCAL; // 开启接收并忽略控制线

    // 设置非阻塞读取
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
        this->declare_parameter<std::string>("port_name", "/dev/ttyUSB0");
        this->declare_parameter<int>("baud_rate", 115200);
    }

    CallbackReturn on_configure(const rclcpp_lifecycle::State &) override {
        dvl_pub_ = this->create_publisher<hal::msg::HalDvl>("/hal/dvl", 10);
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
    std::shared_ptr<rclcpp_lifecycle::LifecyclePublisher<hal::msg::HalDvl>> dvl_pub_;
    rclcpp::TimerBase::SharedPtr publish_timer_;

    hal::msg::HalDvl cached_msg_;
    std::mutex msg_mutex_;

    int serial_fd_ = -1;
    std::thread dvl_thread_;
    std::atomic<bool> is_running_{false};

    void publish_timer_callback() {
        if (dvl_pub_->is_activated()) {
            hal::msg::HalDvl msg_to_publish;
            {
                std::lock_guard<std::mutex> lock(msg_mutex_);
                msg_to_publish = cached_msg_;
            }
            dvl_pub_->publish(msg_to_publish);
        }
    }

    void parse_and_cache(const std::string& data, int64_t capture_time_ns) {
        // 【关键修改 3】找到报文真正的起始位，剔除前方的乱码字节
        size_t start_idx = data.find("wrx");
        if (start_idx == std::string::npos) {
            start_idx = data.find("wrz");
        }
        if (start_idx == std::string::npos) return;

        std::string clean_data = data.substr(start_idx);
        size_t star_pos = clean_data.find('*');
        std::string data_body = (star_pos != std::string::npos) ? clean_data.substr(0, star_pos) : clean_data;
        
        std::vector<std::string> tokens;
        std::stringstream ss(data_body);
        std::string token;
        
        while (std::getline(ss, token, ',')) {
            tokens.push_back(token);
        }

        if (tokens.empty()) return;

        bool is_valid = false;
        float vx = 0.0f, vy = 0.0f, vz = 0.0f;

        try {
            if (tokens[0] == "wrx" && tokens.size() >= 8) {
                std::string valid_str = tokens[7];
                valid_str.erase(valid_str.find_last_not_of(" \n\r\t") + 1); 
                if (valid_str == "y" || valid_str == "Y") {
                    is_valid = true;
                    vx = std::stof(tokens[2]);
                    vy = std::stof(tokens[3]);
                    vz = std::stof(tokens[4]);
                }
            } 
            else if (tokens[0] == "wrz" && tokens.size() >= 5) {
                std::string valid_str = tokens[4];
                valid_str.erase(valid_str.find_last_not_of(" \n\r\t") + 1);
                if (valid_str == "y" || valid_str == "Y") {
                    is_valid = true;
                    vx = std::stof(tokens[1]);
                    vy = std::stof(tokens[2]);
                    vz = std::stof(tokens[3]);
                }
            } else {
                return; 
            }
        } catch (const std::exception& e) {
            RCLCPP_DEBUG(this->get_logger(), "DVL 数据解析转换异常: %s", e.what());
            return;
        }

        std::lock_guard<std::mutex> lock(msg_mutex_);
        cached_msg_.timestamp = capture_time_ns;

        if (is_valid) {
            cached_msg_.velocity_x = vx;
            cached_msg_.velocity_y = vy;
            cached_msg_.velocity_z = vz;
        } else {
            RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 2000, 
                "DVL 底面失锁！已强制速度归零以保护系统");
            cached_msg_.velocity_x = 0.0f;
            cached_msg_.velocity_y = 0.0f;
            cached_msg_.velocity_z = 0.0f;
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
                                        (baud_int == 460800) ? B460800 : B9600; 

                    serial_fd_ = setup_native_uart(port, baud_rate);
                    if (serial_fd_ < 0) {
                        std::this_thread::sleep_for(1s);
                        continue;
                    }
                    //RCLCPP_INFO(this->get_logger(), "DVL 原生串口已打开: %s", port.c_str());
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
                        
                        // 【关键修改 2】仅依据 '\n' 拆分字符串，提升鲁棒性
                        while ((pos = buffer.find('\n', processed_pos)) != std::string::npos) {
                            std::string line = buffer.substr(processed_pos, pos - processed_pos);
                            processed_pos = pos + 1; 

                            // 如果末尾带有 '\r'，将其弹出剥离
                            if (!line.empty() && line.back() == '\r') {
                                line.pop_back();
                            }
                            
                            // 放宽要求，不再强求下标0
                            if (line.find("wrx") != std::string::npos || line.find("wrz") != std::string::npos) {
                                RCLCPP_INFO(this->get_logger(), "收到原始合法串口数据 -> %s", line.c_str());
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