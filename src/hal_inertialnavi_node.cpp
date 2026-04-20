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
#include <sys/select.h>

/**
 * @brief 原生 Linux 串口初始化函数 
 * @param port_name 串口设备路径
 * @param baud_rate 波特率宏定义 (如 B460800)
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
        // 【修改 1】声明两个独立的串口参数，分别用于读取和注入
        this->declare_parameter<std::string>("ins_port_name", "/dev/ttyUSB0"); // 读取导航数据(Port 1)
        this->declare_parameter<std::string>("dvl_port_name", "/dev/ttyUSB1"); // 注入 DVL数据(Port 2)
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
        
        // 安全关闭两个串口
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

    CallbackReturn on_shutdown(const rclcpp_lifecycle::State &) override {
        is_running_ = false;
        if (ins_thread_.joinable()) ins_thread_.join();
        
        if (ins_fd_ >= 0) { close(ins_fd_); ins_fd_ = -1; }
        if (dvl_fd_ >= 0) { close(dvl_fd_); dvl_fd_ = -1; }
        return CallbackReturn::SUCCESS;
    }

private:
    std::shared_ptr<rclcpp_lifecycle::LifecyclePublisher<hal::msg::HalInertialnaviMsg>> ins_pub_;
    rclcpp::Subscription<hal::msg::HalDvlMsg>::SharedPtr dvl_sub_;
    rclcpp::TimerBase::SharedPtr publish_timer_;

    hal::msg::HalInertialnaviMsg cached_msg_;
    std::mutex msg_mutex_;

    // 【修改 1】分离文件描述符
    int ins_fd_ = -1;
    int dvl_fd_ = -1;
    
    std::thread ins_thread_;
    std::atomic<bool> is_running_{false};

    void dvl_callback(const hal::msg::HalDvlMsg::SharedPtr msg) {
        // 向专门的 DVL 注入串口写入数据
        if (dvl_fd_ < 0) return; 

        // 【修改 3】严格遵照 GI510 手册表 4.1，将协议头改为 UZHDT，并补齐 18 个字段
        std::ostringstream ss;
        ss << "UZHDT," << std::fixed << std::setprecision(3)
           << msg->velocity_x << "," << msg->velocity_y << "," << msg->velocity_z
           << ",y,0.0,0.0,0;0;0;0;0;0;0;0;0,0,0.0,0.0,0,0.0,,,";
        
        std::string payload = ss.str();
        uint8_t checksum = 0;
        for (char c : payload) {
            checksum ^= static_cast<uint8_t>(c);
        }
        
        std::ostringstream checksum_ss;
        checksum_ss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(checksum);
        std::string cmd = "$" + payload + "*" + checksum_ss.str() + "\r\n";
        
        int bytes_written = write(dvl_fd_, cmd.c_str(), cmd.length());
        if (bytes_written < 0 && errno != EAGAIN) {
            RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 1000, "向惯导注入 DVL 辅助数据失败");
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

        // 确保字段长度满足要求（至少需要解析到 16 号状态字，对应索引 15）
        if (tokens.size() >= 16) {
            try {
                // 【修改 5】拦截状态机异常数据，保护控制系统
                int status = tokens[15].empty() ? 0 : std::stoi(tokens[15]);
                // 100=待机，201=粗对准 (如果状态不对，直接抛弃数据，不更新缓存)
                if (status == 100 || status == 201) {
                    RCLCPP_WARN_THROTTLE(this->get_logger(), *this->get_clock(), 2000, 
                        "惯导当前未完全对准 (Status: %d)，已抛弃脏数据，防止失控", status);
                    return; 
                }

                float yaw = tokens[3].empty() ? 0.0f : std::stof(tokens[3]);
                if (yaw > 360.0f || yaw < -360.0f) throw std::runtime_error("Yaw 角度越界");

                std::lock_guard<std::mutex> lock(msg_mutex_);
                cached_msg_.timestamp = capture_time_ns;
                
                cached_msg_.yaw   = yaw;
                cached_msg_.pitch = tokens[4].empty() ? 0.0f : std::stof(tokens[4]);
                cached_msg_.roll  = tokens[5].empty() ? 0.0f : std::stof(tokens[5]);
                
                // 【修改 4】彻底修正数组索引 (索引 = 字段号 - 1)
                cached_msg_.latitude  = tokens[6].empty() ? 0.0f : std::stof(tokens[6]);  // 字段 7: Lat
                cached_msg_.longitude = tokens[7].empty() ? 0.0f : std::stof(tokens[7]);  // 字段 8: Lon
                
                cached_msg_.east_velocity  = tokens[9].empty() ? 0.0f : std::stof(tokens[9]);   // 字段 10: Ve
                cached_msg_.north_velocity = tokens[10].empty() ? 0.0f : std::stof(tokens[10]); // 字段 11: Vn
                cached_msg_.sky_velocity   = tokens[11].empty() ? 0.0f : std::stof(tokens[11]); // 字段 12: Vu
                
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
                // 分别尝试打开两个硬件串口
                if (ins_fd_ < 0) {
                    std::string port = this->get_parameter("ins_port_name").as_string();
                    // 【修改 2】设置正确的波特率 B460800
                    ins_fd_ = setup_native_uart(port, B460800);
                    if (ins_fd_ >= 0) {
                        RCLCPP_INFO(this->get_logger(), "惯导读取串口(Port1)已打开: %s", port.c_str());
                    }
                }
                
                if (dvl_fd_ < 0) {
                    std::string port = this->get_parameter("dvl_port_name").as_string();
                    // DVL 注入端口同样为 B460800
                    dvl_fd_ = setup_native_uart(port, B460800);
                    if (dvl_fd_ >= 0) {
                        RCLCPP_INFO(this->get_logger(), "惯导注入串口(Port2)已打开: %s", port.c_str());
                    }
                }

                // 如果读取口依然未打开，休眠并继续尝试
                if (ins_fd_ < 0) {
                    std::this_thread::sleep_for(1s);
                    continue;
                }

                // 仅监听读取端口 (ins_fd_)
                fd_set read_fds;
                FD_ZERO(&read_fds);
                FD_SET(ins_fd_, &read_fds);

                struct timeval tv;
                tv.tv_sec = 0;
                tv.tv_usec = 5000; // 5ms 超时

                int ret = select(ins_fd_ + 1, &read_fds, NULL, NULL, &tv);

                if (ret < 0) {
                    throw std::runtime_error("select 监听底层错误");
                } 
                else if (ret > 0 && FD_ISSET(ins_fd_, &read_fds)) {
                    int bytes_read = read(ins_fd_, read_buf, sizeof(read_buf));
                    
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
                            
                            if (line.rfind("$UZHDR", 0) == 0) {
                                parse_and_cache(line, capture_time_ns);
                            }
                        }
                        
                        if (processed_pos > 0) {
                            buffer.erase(0, processed_pos);
                        }
                        if (buffer.size() > 4096) buffer.clear(); 
                    }
                }
            } catch (const std::exception& e) {
                RCLCPP_ERROR(this->get_logger(), "惯导串口异常: %s", e.what());
                if (ins_fd_ >= 0) { close(ins_fd_); ins_fd_ = -1; }
                if (dvl_fd_ >= 0) { close(dvl_fd_); dvl_fd_ = -1; }
                std::this_thread::sleep_for(2s);
            }
        }
    }
};

int main(int argc, char ** argv) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<HalInertialNaviNode>("hal_inertialnavi_node")->get_node_base_interface());
    rclcpp::shutdown();
    return 0;
}