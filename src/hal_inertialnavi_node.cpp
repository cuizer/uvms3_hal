#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include "hal/msg/hal_inertialnavi.hpp" 
#include "hal/msg/hal_dvl.hpp"          
#include <thread>
#include <mutex>
#include <chrono>
#include <vector>
#include <sstream>
#include <iomanip>
#include <atomic>

#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <cstring>
#include <sys/select.h>

/**
 * @brief 原生 Linux 串口初始化函数 
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

class HalInertialNaviNode : public rclcpp_lifecycle::LifecycleNode
{
public:
    HalInertialNaviNode(const std::string & node_name)
    : rclcpp_lifecycle::LifecycleNode(node_name)
    {
        this->declare_parameter<std::string>("ins_port_name", "/dev/ttyTHS0"); 
        this->declare_parameter<std::string>("dvl_port_name", "/dev/ttyTHS1"); 
        cached_msg_.timestamp = 0;
    }

    CallbackReturn on_configure(const rclcpp_lifecycle::State &) override {
        ins_pub_ = this->create_publisher<hal::msg::HalInertialnavi>("/hal/inertialnavi", 10);
        
        dvl_sub_ = this->create_subscription<hal::msg::HalDvl>(
            "/hal/dvl", 10, std::bind(&HalInertialNaviNode::dvl_callback, this, std::placeholders::_1)
        );

        publish_timer_ = this->create_wall_timer(
            20ms, std::bind(&HalInertialNaviNode::publish_timer_callback, this));

        RCLCPP_INFO(get_logger(), "惯导节点配置完成，等待激活...");
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_activate(const rclcpp_lifecycle::State & state) override {
        ins_pub_->on_activate();
        is_running_ = true;
        ins_thread_ = std::thread(&HalInertialNaviNode::ins_thread_function, this);
        RCLCPP_INFO(get_logger(), "惯导节点已激活，线程已启动。");
        return LifecycleNode::on_activate(state);
    }

    CallbackReturn on_deactivate(const rclcpp_lifecycle::State & state) override {
        ins_pub_->on_deactivate();
        is_running_ = false;
        if (ins_thread_.joinable()) ins_thread_.join();
        
        if (ins_fd_ >= 0) { close(ins_fd_); ins_fd_ = -1; }
        if (dvl_fd_ >= 0) { close(dvl_fd_); dvl_fd_ = -1; }
        
        return LifecycleNode::on_deactivate(state);
    }

    CallbackReturn on_cleanup(const rclcpp_lifecycle::State &) override {
        ins_pub_.reset();
        dvl_sub_.reset();
        publish_timer_.reset();
        return CallbackReturn::SUCCESS;
    }

private:
    std::shared_ptr<rclcpp_lifecycle::LifecyclePublisher<hal::msg::HalInertialnavi>> ins_pub_;
    rclcpp::Subscription<hal::msg::HalDvl>::SharedPtr dvl_sub_;
    rclcpp::TimerBase::SharedPtr publish_timer_;

    hal::msg::HalInertialnavi cached_msg_;
    std::mutex msg_mutex_;

    int ins_fd_ = -1;
    int dvl_fd_ = -1;
    
    std::thread ins_thread_;
    std::atomic<bool> is_running_{false};

    void dvl_callback(const hal::msg::HalDvl::SharedPtr msg) {
        if (dvl_fd_ < 0) return; 

        std::ostringstream ss;
        ss << "UZHDT," << std::fixed << std::setprecision(3)
           << msg->velocity_x << "," << msg->velocity_y << "," << msg->velocity_z
           << ",y,0.0,0.0,0;0;0;0;0;0;0;0;0,0,0.0,0.0,0,0.0,,,";
        
        std::string payload = ss.str();
        uint8_t checksum = 0;
        for (char c : payload) checksum ^= static_cast<uint8_t>(c);
        
        std::ostringstream checksum_ss;
        checksum_ss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(checksum);
        std::string cmd = "$" + payload + "*" + checksum_ss.str() + "\r\n";
        
        write(dvl_fd_, cmd.c_str(), cmd.length());
    }

    void publish_timer_callback() {
        if (ins_pub_->is_activated()) {
            hal::msg::HalInertialnavi msg_to_publish;
            {
                std::lock_guard<std::mutex> lock(msg_mutex_);
                msg_to_publish = cached_msg_;
            }
            // 只要时间戳不为0（表示至少解析成功过一次）就发布
            if (msg_to_publish.timestamp != 0) {
                ins_pub_->publish(msg_to_publish);
            }
        }
    }

    void parse_and_cache(const std::string& data, int64_t capture_time_ns) {
        // 查找正文结束符
        size_t star_pos = data.find('*');
        if (star_pos == std::string::npos) return; 

        // 截取帧头之后的部分
        size_t head_pos = data.find("$UZHDR");
        if (head_pos == std::string::npos) return;

        std::string data_body = data.substr(head_pos + 7, star_pos - (head_pos + 7));
        std::vector<std::string> tokens;
        std::stringstream ss(data_body);
        std::string token;
        
        while (std::getline(ss, token, ',')) tokens.push_back(token);

        // 调试信息：如果收到数据但解析不出 tokens，可以在这里打印
        if (tokens.size() >= 10) {
            try {
                std::lock_guard<std::mutex> lock(msg_mutex_);
                // 优先使用接收时的系统纳秒时间戳，防止硬件时间字段解析错误导致节点不发消息
                cached_msg_.timestamp = capture_time_ns; 
                
                // 索引建议重新核对硬件协议手册，这里沿用您的逻辑并做安全检查
                cached_msg_.yaw       = (tokens.size() > 1) ? std::stof(tokens[1]) : 0.0f;
                cached_msg_.pitch     = (tokens.size() > 2) ? std::stof(tokens[2]) : 0.0f;
                cached_msg_.roll      = (tokens.size() > 3) ? std::stof(tokens[3]) : 0.0f;
                cached_msg_.latitude  = (tokens.size() > 4) ? std::stod(tokens[4]) : 0.0;
                cached_msg_.longitude = (tokens.size() > 5) ? std::stod(tokens[5]) : 0.0;
                cached_msg_.east_velocity  = (tokens.size() > 7) ? std::stof(tokens[7]) : 0.0f;
                cached_msg_.north_velocity = (tokens.size() > 8) ? std::stof(tokens[8]) : 0.0f;
                cached_msg_.sky_velocity   = (tokens.size() > 9) ? std::stof(tokens[9]) : 0.0f;
                
            } catch (const std::exception& e) {
                RCLCPP_WARN(this->get_logger(), "数据转换失败: %s", e.what());
            }
        }
    }

    void ins_thread_function() {
        std::string buffer = "";
        char read_buf[1024]; 

        while (rclcpp::ok() && is_running_) {
            if (ins_fd_ < 0) {
                std::string port = this->get_parameter("ins_port_name").as_string();
                ins_fd_ = setup_native_uart(port, B460800);
                if (ins_fd_ >= 0) RCLCPP_INFO(this->get_logger(), "惯导串口 %s 已打开", port.c_str());
            }
            if (dvl_fd_ < 0) {
                std::string port = this->get_parameter("dvl_port_name").as_string();
                dvl_fd_ = setup_native_uart(port, B460800);
            }

            if (ins_fd_ < 0) {
                std::this_thread::sleep_for(1s);
                continue;
            }

            fd_set read_fds;
            FD_ZERO(&read_fds);
            FD_SET(ins_fd_, &read_fds);
            struct timeval tv {0, 10000}; // 10ms timeout

            int ret = select(ins_fd_ + 1, &read_fds, NULL, NULL, &tv);

            if (ret > 0 && FD_ISSET(ins_fd_, &read_fds)) {
                int bytes_read = read(ins_fd_, read_buf, sizeof(read_buf));
                if (bytes_read > 0) {
                    int64_t capture_time_ns = this->now().nanoseconds();
                    buffer.append(read_buf, bytes_read);
                    
                    size_t pos = 0;
                    while ((pos = buffer.find("\r\n")) != std::string::npos) {
                        std::string line = buffer.substr(0, pos);
                        buffer.erase(0, pos + 2);
                        
                        if (line.find("$UZHDR") != std::string::npos) {
                            parse_and_cache(line, capture_time_ns);
                        }
                    }
                    if (buffer.size() > 4096) buffer.clear(); 
                }
            }
        }
    }
};

int main(int argc, char ** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<HalInertialNaviNode>("hal_inertialnavi_node");
    
    // 使用 MultiThreadedExecutor 以便在 Lifecycle 激活前也能响应部分服务
    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(node->get_node_base_interface());
    executor.spin();
    
    rclcpp::shutdown();
    return 0;
}