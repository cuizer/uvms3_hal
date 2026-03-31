#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include "hal/msg/hal_inertialnavi_msg.hpp" 
#include "hal/msg/hal_dvl_msg.hpp"          
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <sstream>
#include <iomanip>
#include <atomic>

// 引入 Linux 底层串口所需的系统头文件
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <sys/ioctl.h>

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
    fcntl(fd, F_SETFL, 0);

    return fd;
}

using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;
using namespace std::chrono_literals;

class HalInertialNaviNode : public rclcpp_lifecycle::LifecycleNode
{
public:
    HalInertialNaviNode(const std::string & node_name)
    : rclcpp_lifecycle::LifecycleNode(node_name)
    {
        this->declare_parameter<std::string>("port_name", "/dev/ttyUSB1");
    }

    CallbackReturn on_configure(const rclcpp_lifecycle::State &) override {
        ins_pub_ = this->create_publisher<hal::msg::HalInertialnaviMsg>("hal_inertialnavi_msg", 10);
        
        dvl_sub_ = this->create_subscription<hal::msg::HalDvlMsg>(
            "hal_dvl_msg", 10, std::bind(&HalInertialNaviNode::dvl_callback, this, std::placeholders::_1)
        );

        publish_timer_ = this->create_wall_timer(
            20ms, std::bind(&HalInertialNaviNode::publish_timer_callback, this));

        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_activate(const rclcpp_lifecycle::State & state) override {
        ins_pub_->on_activate();
        is_running_ = true;
        ins_thread_ = std::thread(&HalInertialNaviNode::ins_thread_function, this);
        return LifecycleNode::on_activate(state);
    }

    CallbackReturn on_deactivate(const rclcpp_lifecycle::State & state) override {
        ins_pub_->on_deactivate();
        is_running_ = false;
        if (ins_thread_.joinable()) ins_thread_.join();
        
        if (serial_fd_ >= 0) {
            close(serial_fd_);
            serial_fd_ = -1;
        }
        return LifecycleNode::on_deactivate(state);
    }

    CallbackReturn on_cleanup(const rclcpp_lifecycle::State &) override {
        ins_pub_.reset();
        dvl_sub_.reset();
        publish_timer_.reset();
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_shutdown(const rclcpp_lifecycle::State &) override {
        is_running_ = false;
        if (ins_thread_.joinable()) ins_thread_.join();
        
        if (serial_fd_ >= 0) {
            close(serial_fd_);
            serial_fd_ = -1;
        }
        return CallbackReturn::SUCCESS;
    }

private:
    std::shared_ptr<rclcpp_lifecycle::LifecyclePublisher<hal::msg::HalInertialnaviMsg>> ins_pub_;
    rclcpp::Subscription<hal::msg::HalDvlMsg>::SharedPtr dvl_sub_;
    rclcpp::TimerBase::SharedPtr publish_timer_;

    hal::msg::HalInertialnaviMsg cached_msg_;
    std::mutex msg_mutex_;

    int serial_fd_ = -1;
    std::thread ins_thread_;
    std::atomic<bool> is_running_{false};

    void dvl_callback(const hal::msg::HalDvlMsg::SharedPtr msg) {
        if (serial_fd_ < 0) return; 

        std::ostringstream ss;
        ss << "UZDVL," << std::fixed << std::setprecision(3)
           << msg->velocity_x << "," << msg->velocity_y << "," << msg->velocity_z;
        
        std::string payload = ss.str();
        uint8_t checksum = 0;
        for (char c : payload) {
            checksum ^= static_cast<uint8_t>(c);
        }
        
        std::ostringstream checksum_ss;
        checksum_ss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(checksum);
        std::string cmd = "$" + payload + "*" + checksum_ss.str() + "\r\n";
        
        int bytes_written = write(serial_fd_, cmd.c_str(), cmd.length());
        if (bytes_written < 0) {
            RCLCPP_WARN(this->get_logger(), "向惯导写入 DVL 辅助数据失败");
        }
    }

    void publish_timer_callback() {
        if (ins_pub_->is_activated()) {
            hal::msg::HalInertialnaviMsg msg_to_publish;
            {
                std::lock_guard<std::mutex> lock(msg_mutex_);
                msg_to_publish = cached_msg_;
            }
            ins_pub_->publish(msg_to_publish);
        }
    }

    void parse_and_cache(const std::string& data, int64_t capture_time_ns) {
        size_t star_pos = data.find('*');
        if (star_pos == std::string::npos) return; 

        std::string data_body = data.substr(0, star_pos);
        std::vector<std::string> tokens;
        std::stringstream ss(data_body);
        std::string token;
        
        while (std::getline(ss, token, ',')) {
            tokens.push_back(token);
        }

        if (tokens.size() >= 14) {
            try {
                float yaw = tokens[3].empty() ? 0.0f : std::stof(tokens[3]);
                if (yaw > 360.0f || yaw < -360.0f) throw std::runtime_error("Yaw 角度越界");

                std::lock_guard<std::mutex> lock(msg_mutex_);
                cached_msg_.timestamp = capture_time_ns;
                
                // 姿态数据
                cached_msg_.yaw   = yaw;
                cached_msg_.pitch = tokens[4].empty() ? 0.0f : std::stof(tokens[4]);
                cached_msg_.roll  = tokens[5].empty() ? 0.0f : std::stof(tokens[5]);
                
                // 位置数据
                cached_msg_.latitude  = tokens[8].empty() ? 0.0f : std::stof(tokens[8]);
                cached_msg_.longitude = tokens[9].empty() ? 0.0f : std::stof(tokens[9]);
                
                // 【关键修复】使用你 msg 文件里定义的真实变量名：east_velocity, north_velocity, sky_velocity
                cached_msg_.east_velocity  = tokens[11].empty() ? 0.0f : std::stof(tokens[11]);
                cached_msg_.north_velocity = tokens[12].empty() ? 0.0f : std::stof(tokens[12]);
                cached_msg_.sky_velocity   = tokens[13].empty() ? 0.0f : std::stof(tokens[13]);
                
            } catch (const std::exception& e) {
                RCLCPP_DEBUG(this->get_logger(), "惯导脏数据解析跳过: %s", e.what());
            }
        }
    }

    void ins_thread_function() {
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
                    RCLCPP_INFO(this->get_logger(), "惯导原生串口已打开: %s", port.c_str());
                }

                int available_bytes = 0;
                if (ioctl(serial_fd_, FIONREAD, &available_bytes) < 0) {
                    throw std::runtime_error("底层 ioctl 错误，串口可能断开");
                }

                if (available_bytes > 0) {
                    int64_t capture_time_ns = this->now().nanoseconds();
                    int bytes_to_read = std::min(available_bytes, (int)sizeof(read_buf));
                    
                    int bytes_read = read(serial_fd_, read_buf, bytes_to_read);
                    if (bytes_read < 0) throw std::runtime_error("原生 read 失败");

                    buffer.append(read_buf, bytes_read);
                    
                    size_t pos = 0;
                    while ((pos = buffer.find("\r\n")) != std::string::npos) {
                        std::string line = buffer.substr(0, pos);
                        buffer.erase(0, pos + 2);
                        
                        if (line.rfind("$UZHDR", 0) == 0) {
                            parse_and_cache(line, capture_time_ns);
                        }
                    }
                    if (buffer.size() > 4096) buffer.clear(); 
                }
            } catch (const std::exception& e) {
                RCLCPP_ERROR(this->get_logger(), "惯导异常: %s", e.what());
                if (serial_fd_ >= 0) {
                    close(serial_fd_);
                    serial_fd_ = -1;
                }
                std::this_thread::sleep_for(2s);
            }
            std::this_thread::sleep_for(5ms); 
        }
    }
};

int main(int argc, char ** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<HalInertialNaviNode>("hal_inertialnavi_node")->get_node_base_interface());
    rclcpp::shutdown();
    return 0;
}