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
        this->declare_parameter<std::string>("ins_port_name", "/dev/ttyUART_485_422_B"); 
        this->declare_parameter<std::string>("dvl_port_name", "/dev/ttyUART_485_422_A"); 
        cached_msg_.timestamp = 0;
        cached_msg_.connection_status = 0;
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
    std::atomic<int64_t> last_ins_data_ns_{0};

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

            int64_t now_ns = this->now().nanoseconds();
            int64_t last_ns = last_ins_data_ns_.load();
            if (last_ns == 0 || (now_ns - last_ns) > 2000000000ULL) {
                msg_to_publish.connection_status = 0;
            }

            if (msg_to_publish.timestamp != 0) {
                ins_pub_->publish(msg_to_publish);
            }
        }
    }

    void parse_and_cache(const std::string& data, int64_t capture_time_ns) {
        size_t star_pos = data.find('*');
        if (star_pos == std::string::npos) return; 

        size_t head_pos = data.find("$UZHDR");
        if (head_pos == std::string::npos) return;

        std::string data_body = data.substr(head_pos + 7, star_pos - (head_pos + 7));
        std::vector<std::string> tokens;
        std::stringstream ss(data_body);
        std::string token;
        
        while (std::getline(ss, token, ',')) tokens.push_back(token);

        if (tokens.size() >= 10) {
            try {
                std::lock_guard<std::mutex> lock(msg_mutex_);
                cached_msg_.timestamp = capture_time_ns;
                cached_msg_.connection_status = 1;
                last_ins_data_ns_.store(capture_time_ns); 
                
                // 【修复】：严格按照 GI510 协议表 4.2 进行数组下标映射
                cached_msg_.yaw       = (tokens.size() > 2) ? std::stof(tokens[2]) : 0.0f;  // Heading
                cached_msg_.pitch     = (tokens.size() > 3) ? std::stof(tokens[3]) : 0.0f;  // Pitch
                cached_msg_.roll      = (tokens.size() > 4) ? std::stof(tokens[4]) : 0.0f;  // Roll
                cached_msg_.latitude  = (tokens.size() > 5) ? std::stod(tokens[5]) : 0.0;   // Latitude
                cached_msg_.longitude = (tokens.size() > 6) ? std::stod(tokens[6]) : 0.0;   // Longitude

                // 注意：tokens[7] 是深度 (Depth)，这里跳过，直接取第 8,9,10 个数据为速度
                cached_msg_.east_velocity  = (tokens.size() > 8) ? std::stof(tokens[8]) : 0.0f;   // Ve
                cached_msg_.north_velocity = (tokens.size() > 9) ? std::stof(tokens[9]) : 0.0f;   // Vn
                cached_msg_.sky_velocity   = (tokens.size() > 10) ? std::stof(tokens[10]) : 0.0f; // Vu
                
                // 成功解析打印，测试成功后可注释
                // RCLCPP_INFO(this->get_logger(), "【探针5】解析成功并缓存! Yaw: %.2f, Lat: %.6f", cached_msg_.yaw, cached_msg_.latitude);

            } catch (const std::exception& e) {
                // 如果抛出异常，说明某个字段的数据不是合法数字
                RCLCPP_ERROR(this->get_logger(), "【探针4】致命错误：数据类型转换失败: %s", e.what());
            }
        }
        // } else {
        //     RCLCPP_WARN(this->get_logger(), "【探针】切分的字段数量不足！实际数量: %zu", tokens.size());
        // }
    }

    void ins_thread_function() {
        std::string buffer = "";
        char read_buf[1024]; 

        while (rclcpp::ok() && is_running_) {
            if (ins_fd_ < 0) {
                std::string port = this->get_parameter("ins_port_name").as_string();
                ins_fd_ = setup_native_uart(port, B460800);
                if (ins_fd_ >= 0) {
                    RCLCPP_INFO(this->get_logger(), "惯导串口 %s 已打开", port.c_str());
                } else {
                    // 【修复】：如果权限不足，每两秒打印一次报警，避免静默失败
                    RCLCPP_ERROR_THROTTLE(this->get_logger(), *this->get_clock(), 2000, 
                        "惯导串口 %s 打开失败！请检查连接或执行 sudo chmod 777 %s", 
                        port.c_str(), port.c_str());
                }
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
                    
                    // ================= [探针 1: 打印刚从底层读到的原始字节] =================
                    // std::string raw_str(read_buf, bytes_read);
                    // RCLCPP_INFO(this->get_logger(), "【探针1】底层收到 %d 字节数据: %s", bytes_read, raw_str.c_str());

                    size_t pos = 0;
                    // 【修复】：改为寻找 '\n'，兼容 '\r\n' 和只有 '\n' 的情况
                    while ((pos = buffer.find('\n')) != std::string::npos) {
                        std::string line = buffer.substr(0, pos);
                        
                        // 把末尾可能存在的 '\r' 削掉
                        if (!line.empty() && line.back() == '\r') {
                            line.pop_back(); 
                        }
                        
                        buffer.erase(0, pos + 1); // 加上 '\n' 本身的长度1
                        
                        if (line.find("$UZHDR") != std::string::npos) {
                            // ================= [探针 2: 确认进入解析流程] =================
                            // RCLCPP_INFO(this->get_logger(), "【探针2】匹配到头部，整行数据为: %s", line.c_str());
                            parse_and_cache(line, capture_time_ns);
                        }
                    }
                    if (buffer.size() > 4096) {
                        RCLCPP_WARN(this->get_logger(), "【警告】Buffer满4096被清空！可能是换行符异常。当前Buffer: %s", buffer.c_str());
                        buffer.clear(); 
                    }
                }
            }
        }
    }
};

int main(int argc, char ** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<HalInertialNaviNode>("hal_inertialnavi_node");
    
    // // 【修复】：强制自动触发 Configure 和 Activate，摆脱手动 Lifecycle 激活的陷阱
    // node->configure();
    // node->activate();
    
    rclcpp::executors::MultiThreadedExecutor executor;
    executor.add_node(node->get_node_base_interface());
    executor.spin();
    
    rclcpp::shutdown();
    return 0;
}