#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic>
#include <mutex>
#include <array>
#include <cmath>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h> 
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <cstring>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "rclcpp_lifecycle/lifecycle_publisher.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "std_msgs/msg/float64_multi_array.hpp" 
#include "std_srvs/srv/set_bool.hpp" 

#include "hal/msg/hal_mainthruster.hpp"
#include "hal/msg/hal_auxithruster.hpp"
#include "hal/srv/hal_thrustercontrol_srv.hpp"

using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;
using std::placeholders::_1;
using std::placeholders::_2;

class HalThrusterNode : public rclcpp_lifecycle::LifecycleNode {
public:
    explicit HalThrusterNode(const std::string & node_name, bool intra_process_comms = false)
    : rclcpp_lifecycle::LifecycleNode(node_name,
        rclcpp::NodeOptions().use_intra_process_comms(intra_process_comms)) {}

    CallbackReturn on_configure(const rclcpp_lifecycle::State &) override {
        RCLCPP_INFO(get_logger(), "配置中... 初始化推进器节点接口。");
        pub_main_status_ = this->create_publisher<hal::msg::HalMainthruster>("/hal/mainthruster", 10);
        pub_aux_status_ = this->create_publisher<hal::msg::HalAuxithruster>("/hal/auxithruster", 10);

        srv_control_ = this->create_service<hal::srv::HalThrustercontrolSrv>(
            "/hal/thrustercontrol", std::bind(&HalThrusterNode::control_srv_callback, this, _1, _2));

        srv_estop_ = this->create_service<std_srvs::srv::SetBool>(
            "hal_thruster_estop", std::bind(&HalThrusterNode::estop_callback, this, _1, _2));

        sub_cmd_ = this->create_subscription<std_msgs::msg::Float64MultiArray>(
            "/hal/thruster/cmd", 10, std::bind(&HalThrusterNode::cmd_callback, this, _1));

        // 20ms 定时器：主推轮询、辅推控制下发、话题发布
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(20), std::bind(&HalThrusterNode::timer_general_callback, this));

        hardware_api_init_can("can3");
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_activate(const rclcpp_lifecycle::State & state) override {
        RCLCPP_INFO(get_logger(), "推进器节点已激活。");
        pub_main_status_->on_activate();
        pub_aux_status_->on_activate();
        
        is_testing_ = false;
        is_estopped_ = false; 
        is_emergency_ascending_ = false;

        // 激活时清空主推和辅推历史状态，避免重新激活后短时间发布旧数据
        real_main_rpm_.store(0);
        real_main_current_raw_.store(0);
        real_main_voltage_.store(0);
        real_main_fault_.store(0);

        for (int i = 0; i < 5; ++i) {
            aux_target_pct_[i].store(0.0);
            aux_rpm_[i].store(0);
            aux_current_[i].store(0.0f);
            aux_voltage_[i].store(0);
            aux_temp_[i].store(0);
            aux_status_machine_[i].store(0);
            aux_fault_[i].store(0);
            last_seen_ms_[i].store(0);
        }
        
        keep_running_ = false; 
        if (can_rx_thread_.joinable()) can_rx_thread_.join();
        
        keep_running_ = true;  
        can_rx_thread_ = std::thread(&HalThrusterNode::can_receive_loop, this);
        
        LifecycleNode::on_activate(state);
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_cleanup(const rclcpp_lifecycle::State &) override {
        pub_main_status_.reset(); pub_aux_status_.reset();
        srv_control_.reset(); srv_estop_.reset(); sub_cmd_.reset(); timer_.reset();
        
        keep_running_ = false; 
        is_testing_ = false;
        if (can_rx_thread_.joinable()) can_rx_thread_.join();
        if (test_thread_.joinable()) test_thread_.join();
        
        hardware_api_close_can(); 
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_deactivate(const rclcpp_lifecycle::State & state) override {
        pub_main_status_->on_deactivate();
        pub_aux_status_->on_deactivate();
        stop_all_thrusters();
        LifecycleNode::on_deactivate(state);

        keep_running_ = false;
        is_testing_ = false;
        if (can_rx_thread_.joinable()) can_rx_thread_.join();
        if (test_thread_.joinable()) test_thread_.join();
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_shutdown(const rclcpp_lifecycle::State &) override {
        stop_all_thrusters();
        hardware_api_close_can();
        return CallbackReturn::SUCCESS;
    }

private:
    // --- 常量定义 ---
    // 主推（逻辑推进器1）实际控制 CAN ID 为 0x301；按原协议 0x300+节点号 / 0x280+节点号，
    // 对应节点号为 1，因此反馈通常为 0x281。接收端同时兼容 0x301 回传。
    const uint32_t MAIN_THRUSTER_CAN_ID = 0x301;
    const uint32_t MAIN_THRUSTER_RX_ID = 0x281;
    const int64_t ONLINE_TIMEOUT_MS = 4000; // 4秒未收到反馈判定为离线
    
    // 推进器逻辑编号与实际硬件映射：
    // 推进器1 = 主推  -> CAN ID 0x301
    // 推进器2 = 辅推1 -> ESC ID2
    // 推进器3 = 辅推2 -> ESC ID3
    // 推进器4 = 辅推3 -> ESC ID4
    // 推进器5 = 辅推4 -> ESC ID5
    // 推进器6 = 辅推5 -> ESC ID6
    // 因此内部 5 路辅推数组 index 0~4 分别对应逻辑推进器2~6。
    const std::array<uint8_t, 5> ACTIVE_AUX_IDS = {2, 3, 4, 5, 6};

    // --- 状态控制变量 ---
    std::atomic<bool> is_estopped_{false}; 
    std::atomic<bool> is_testing_{false}; 
    std::atomic<bool> is_emergency_ascending_{false};
    std::atomic<int> can_socket_{-1};
    std::mutex can_socket_mutex_;

    std::thread test_thread_;
    std::thread can_rx_thread_;
    std::atomic<bool> keep_running_{false}; 

    // --- 数据缓存：主推进器 ---
    std::atomic<int32_t> real_main_rpm_{0};
    std::atomic<int32_t> real_main_current_raw_{0}; 
    std::atomic<int32_t> real_main_voltage_{0};     
    std::atomic<uint32_t> real_main_fault_{0};
    
    // --- 数据缓存：5 路辅助推进器（Index 0~4 对应逻辑推进器2~6 / ESC ID2~6）---
    std::array<std::atomic<double>, 5> aux_target_pct_{};
    std::array<std::atomic<int16_t>, 5> aux_rpm_{};
    std::array<std::atomic<float>, 5>   aux_current_{};
    std::array<std::atomic<uint8_t>, 5> aux_voltage_{};
    std::array<std::atomic<int8_t>, 5>  aux_temp_{};
    std::array<std::atomic<uint8_t>, 5> aux_status_machine_{};
    std::array<std::atomic<uint8_t>, 5> aux_fault_{};
    std::array<std::atomic<int64_t>, 5> last_seen_ms_{};

    // --- ROS2 接口指针 ---
    std::shared_ptr<rclcpp_lifecycle::LifecyclePublisher<hal::msg::HalMainthruster>> pub_main_status_;
    std::shared_ptr<rclcpp_lifecycle::LifecyclePublisher<hal::msg::HalAuxithruster>> pub_aux_status_;
    rclcpp::Service<hal::srv::HalThrustercontrolSrv>::SharedPtr srv_control_;
    rclcpp::Service<std_srvs::srv::SetBool>::SharedPtr srv_estop_; 
    rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr sub_cmd_;
    rclcpp::TimerBase::SharedPtr timer_;

    // --- 辅助工具：CAN ID 映射为内存数组 Index ---
    // 输入实际 ESC ID，返回辅推数组索引；ID2~ID6 -> index0~4，其他返回 -1
    int get_aux_index(uint8_t aux_id) {
        for (size_t i = 0; i < ACTIVE_AUX_IDS.size(); ++i) {
            if (ACTIVE_AUX_IDS[i] == aux_id) return static_cast<int>(i);
        }
        return -1;
    }

    void estop_callback(
        const std::shared_ptr<std_srvs::srv::SetBool::Request> request,
        std::shared_ptr<std_srvs::srv::SetBool::Response> response)
    {
        if (request->data) {
            is_estopped_ = true;
            is_testing_ = false;   
            is_emergency_ascending_ = false;
            stop_all_thrusters();  
            RCLCPP_FATAL(get_logger(), "�� 触发急停！已切断所有控制指令并停止所有推进器（含5路辅推）。");
            response->success = true; response->message = "急停已激活，全系统锁定。";
        } else {
            is_estopped_ = false;
            RCLCPP_INFO(get_logger(), "✅ 急停已解除，恢复推进器控制。");
            response->success = true; response->message = "急停已解除。";
        }
    }

    void control_srv_callback(
        const std::shared_ptr<hal::srv::HalThrustercontrolSrv::Request> request,
        std::shared_ptr<hal::srv::HalThrustercontrolSrv::Response> response) 
    {
        if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) {
            response->success = false; response->message = "节点未激活，无法执行指令。"; return;
        }

        uint8_t cmd = request->command;

        if (is_estopped_ && cmd != 0x04 && cmd != 0x03) {
            response->success = false; 
            response->message = "当前处于急停锁定状态，拒绝执行任何指令！请先发送 0x04 解除急停。"; 
            return;
        }

        switch (cmd) {
            case 0x01: // 主推自检
                if (is_emergency_ascending_ || is_testing_) {
                    response->success = false; response->message = "拒绝：处于安全锁或自检中。"; return;
                }
                if (test_thread_.joinable()) test_thread_.join(); 
                test_thread_ = std::thread(&HalThrusterNode::execute_test_sequence, this, true);
                response->success = true; response->message = "主推自检启动";
                break;

            case 0x02: // 辅推自检
                if (is_emergency_ascending_ || is_testing_) {
                    response->success = false; response->message = "拒绝：处于安全锁或自检中。"; return;
                }
                if (test_thread_.joinable()) test_thread_.join(); 
                test_thread_ = std::thread(&HalThrusterNode::execute_test_sequence, this, false);
                response->success = true; response->message = "辅推自检启动";
                break;

            case 0x03: // 全局急停
                is_estopped_ = true; is_testing_ = false; is_emergency_ascending_ = false;
                if (test_thread_.joinable()) test_thread_.detach();
                stop_all_thrusters(); 
                response->success = true; response->message = "急停已生效";
                break;

            case 0x04: // 解除急停
                is_estopped_ = false;
                response->success = true; response->message = "急停已解除";
                break;

            case 0x05: // 紧急上浮
                is_testing_ = false; 
                if (test_thread_.joinable()) test_thread_.detach();
                execute_emergency_ascent();
                response->success = true; response->message = "紧急上浮序列强制触发。";
                break;

            default:
                response->success = false; response->message = "未知协议代码";
                break;
        }
    }

    void execute_test_sequence(bool is_main_thruster) {
        is_testing_ = true; 
        double test_thrust_pct = 5.0; // 5% 速度测试
        
        if (is_estopped_ || !is_testing_) { is_testing_ = false; return; }

        // 1. 正转
        if (is_main_thruster) {
            set_thruster_rpm_hardware(MAIN_THRUSTER_CAN_ID, test_thrust_pct);
        } else {
            for (int i = 0; i < 5; ++i) aux_target_pct_[i].store(test_thrust_pct);
        }
        if (!interruptible_sleep(3000)) { is_testing_ = false; return; }

        // 2. 反转
        if (is_main_thruster) {
            set_thruster_rpm_hardware(MAIN_THRUSTER_CAN_ID, -test_thrust_pct);
        } else {
            for (int i = 0; i < 5; ++i) aux_target_pct_[i].store(-test_thrust_pct);
        }
        if (!interruptible_sleep(3000)) { is_testing_ = false; return; }

        // 3. 停机
        if (is_main_thruster) {
            set_thruster_rpm_hardware(MAIN_THRUSTER_CAN_ID, 0.0);
        } else {
            for (int i = 0; i < 5; ++i) aux_target_pct_[i].store(0.0);
        }
        is_testing_ = false; 
    }
        
    void execute_emergency_ascent() {
        is_emergency_ascending_ = true;
        set_thruster_rpm_hardware(MAIN_THRUSTER_CAN_ID, 0.0);
        
        // 此处可结合 CI-AUV 的物理分布定向供能
        for (int i = 0; i < 5; ++i) { aux_target_pct_[i].store(0.0); }
        RCLCPP_FATAL(get_logger(), "[紧急上浮动作序列] 控制总线接管就绪。");
    }

    void cmd_callback(const std_msgs::msg::Float64MultiArray::SharedPtr msg) {
        if (this->get_current_state().id() !=
            lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE)
            return;

        if (is_estopped_ || is_testing_ || is_emergency_ascending_)
            return;

        if (msg->data.size() < 6) {
            RCLCPP_WARN(
                get_logger(),
                "推进器控制指令长度不足：收到 %zu，期望至少 6",
                msg->data.size());
            return;
        }

        // 上层控制指令固定按照逻辑推进器 1、2、3、4、5、6 排列：
        // msg->data[0] = 推进器1 = 主推  = CAN ID 0x301
        // msg->data[1] = 推进器2 = 辅推1 = ESC ID2
        // msg->data[2] = 推进器3 = 辅推2 = ESC ID3
        // msg->data[3] = 推进器4 = 辅推3 = ESC ID4
        // msg->data[4] = 推进器5 = 辅推4 = ESC ID5
        // msg->data[5] = 推进器6 = 辅推5 = ESC ID6

        // 推进器1：主推
        set_thruster_rpm_hardware(
            MAIN_THRUSTER_CAN_ID,
            msg->data[0]);

        // 推进器2~6：5 路辅推，内部 index 0~4 对应 ESC ID2~ID6
        for (size_t i = 0; i < 5; ++i) {
            aux_target_pct_[i].store(msg->data[i + 1]);
        }
    }

    void stop_all_thrusters() {
        set_thruster_rpm_hardware(MAIN_THRUSTER_CAN_ID, 0.0); 
        for (int i = 0; i < 5; ++i) {
            aux_target_pct_[i].store(0.0);
        }
        send_aux_control_commands();
    }

    uint16_t calculate_aux_cmd_word(double pct) {
        if (std::abs(pct) < 1e-3) return 1000; 
        uint16_t direction = 0;
        if (pct < 0) { direction = 1 << 11; pct = -pct; }
        if (pct > 100.0) pct = 100.0;
        uint16_t throttle = 1100 + static_cast<uint16_t>((pct / 100.0) * 900.0);
        if (throttle > 2000) throttle = 2000;
        return throttle | direction;
    }

    // --- 精确组装辅推控制帧 (处理 ID 跳跃) ---

    void send_aux_control_commands() {
        int fd;
        {
            std::lock_guard<std::mutex> lock(can_socket_mutex_);
            fd = can_socket_.load();
        }

        if (fd < 0) return;

        // 厂家协议固定分组：
        // 0x200 → 电调 ID0~ID3
        // 0x201 → 电调 ID4~ID7
        // 本机实际使用辅推 ID2~ID6，因此 ID0、ID1、ID7 槽位写停止值
        for (int group = 0; group < 2; ++group) {

            struct can_frame frame;
            frame.can_id = 0x200 + group;
            frame.can_dlc = 8;
            std::memset(frame.data, 0, 8);

            for (int i = 0; i < 4; ++i) {

            // 厂家协议从电调 ID0 开始分槽：
            // group=0 -> ID0,1,2,3；group=1 -> ID4,5,6,7
            uint8_t target_id =
                static_cast<uint8_t>(group * 4 + i);

            int mem_idx = get_aux_index(target_id);

            uint16_t cmd_word;

                if (mem_idx >= 0) {

                    double target_pct =
                        is_estopped_
                        ? 0.0
                        : aux_target_pct_[mem_idx].load();

                    cmd_word =
                        calculate_aux_cmd_word(target_pct);

                } else {

                    // ID0、ID1、ID7 不属于当前 5 个辅推，写停止值 1000
                    cmd_word = 1000;
                }

                frame.data[i * 2] =
                    cmd_word & 0xFF;

                frame.data[i * 2 + 1] =
                    (cmd_word >> 8) & 0xFF;
            }

            ssize_t bytes_written =
                write(fd, &frame, sizeof(struct can_frame));

            if (bytes_written < 0) {

                if (errno == ENETDOWN ||
                    errno == ENODEV ||
                    errno == EBADF) {

                    hardware_api_close_can();
                    break;
                }
            }
        }
    }
    void set_thruster_rpm_hardware(uint32_t target_can_id, double thrust_percentage) {
        int fd;
        {
            std::lock_guard<std::mutex> lock(can_socket_mutex_);
            fd = can_socket_.load();
        }
        if (fd < 0) return;

        if (thrust_percentage > 100.0) thrust_percentage = 100.0;
        if (thrust_percentage < -100.0) thrust_percentage = -100.0;
    
        // target_can_id 直接传入实际主推 CAN ID（0x301），不再重复加 0x300
        uint32_t can_id = target_can_id;
        int32_t target_thrust = static_cast<int32_t>(thrust_percentage);
        struct can_frame frame;
        frame.can_id = can_id; 
        frame.can_dlc = 8; 
    
        frame.data[0] = 0x54; 
        frame.data[1] = 0x43; 
        frame.data[2] = 0x00; 
        frame.data[3] = 0x00; 
        frame.data[4] = (target_thrust >> 24) & 0xFF;
        frame.data[5] = (target_thrust >> 16) & 0xFF;
        frame.data[6] = (target_thrust >> 8) & 0xFF;
        frame.data[7] = target_thrust & 0xFF;
    
        ssize_t bytes_written = write(fd, &frame, sizeof(struct can_frame));
        if (bytes_written < 0) {
            if (errno == ENETDOWN || errno == ENODEV || errno == EBADF) {
                hardware_api_close_can();
            }
        }
    }

    // --- 纯净的 50Hz 遥测数据截获 ---
    void can_receive_loop() {
        struct can_frame frame;
        // 主推（逻辑推进器1）控制 ID 为 0x301；按原协议其标准反馈 ID 为 0x281。
        // 部分固件也可能直接用 0x301 回传，因此下面同时兼容两种情况。
        uint32_t target_main_rx_id = MAIN_THRUSTER_RX_ID;

        while (keep_running_) {
            int current_fd;
            {
                std::lock_guard<std::mutex> lock(can_socket_mutex_);
                current_fd = can_socket_.load();
            }
            if (current_fd < 0) {
                if (hardware_api_init_can("can3")) {
                    RCLCPP_INFO(get_logger(), "�� CAN 总线重连恢复工作！");
                } else {
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
                continue;
            }

            ssize_t nbytes = recv(current_fd, &frame, sizeof(struct can_frame), 0);

            if (nbytes < 0) {
                if (errno == ENETDOWN || errno == ENODEV || errno == EBADF) {
                    // 仅在 current_fd 仍是当前 socket 时才关闭，防止误关新 socket
                    std::lock_guard<std::mutex> lock(can_socket_mutex_);
                    if (can_socket_.load() == current_fd) {
                        int old_fd = can_socket_.exchange(-1);
                        if (old_fd >= 0) { ::close(old_fd); }
                    }
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }
                continue;
            }

            if (nbytes == sizeof(struct can_frame)) {
                uint32_t clean_id = frame.can_id & CAN_SFF_MASK;
                
                // 1. 主推解析
                if ((clean_id == target_main_rx_id || clean_id == MAIN_THRUSTER_CAN_ID) && frame.can_dlc == 8) {
                    int32_t value = (frame.data[4] << 24) | (frame.data[5] << 16) | (frame.data[6] << 8) | frame.data[7];
                    if (frame.data[0] == 0x51) {
                        switch (frame.data[1]) {
                            case 0x56: real_main_rpm_ = value; break;
                            case 0x43: real_main_current_raw_ = value; break;
                            case 0x50: real_main_voltage_ = value; break;
                        }
                    } else if (frame.data[0] == 0x45 && frame.data[1] == 0x46) {
                        real_main_fault_ = static_cast<uint32_t>(value);
                    }
                }
                
                // 2. 辅推 50Hz 自主流解析 (依赖映射函数精准过滤无关报文)
                else if (clean_id >= 0x302 && clean_id <= 0x306 && frame.can_dlc == 8) {
                    // 辅推遥测 CAN ID = 0x300 + 电调编号。
                    // 当前有效 ESC ID 为 2、3、4、5、6，对应逻辑推进器2~6。
                    uint8_t aux_id = clean_id - 0x300; 
                    int mem_idx = get_aux_index(aux_id);
                    
                    if (mem_idx != -1) { // 成功映射到索引，证明这是我们要的实装节点
                        int16_t raw_speed   = static_cast<int16_t>((frame.data[1] << 8) | frame.data[0]);
                        int16_t raw_current = static_cast<int16_t>((frame.data[3] << 8) | frame.data[2]);
                        uint8_t raw_volt    = frame.data[4];
                        int8_t  raw_temp    = frame.data[5];
                        uint8_t raw_status  = frame.data[6];
                        uint8_t raw_fault   = frame.data[7];

                        aux_rpm_[mem_idx].store(raw_speed); 
                        aux_current_[mem_idx].store(static_cast<float>(raw_current) * 0.01f); 
                        aux_voltage_[mem_idx].store(raw_volt); 
                        aux_temp_[mem_idx].store(raw_temp);     
                        aux_status_machine_[mem_idx].store(raw_status);
                        aux_fault_[mem_idx].store(raw_fault);

                        int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                            std::chrono::steady_clock::now().time_since_epoch()).count();
                        last_seen_ms_[mem_idx].store(now_ms); // 打破超时锁死的关键
                    }
                }
            }
        }
    }

    void timer_general_callback() {
        if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) return;

        static int ticks = 0;
        ticks++;

        // 1. 仅对主推进行请求轮询
        switch (ticks % 4) {
            case 0: request_thruster_status(MAIN_THRUSTER_CAN_ID, 0x51, 0x56); break; 
            case 1: request_thruster_status(MAIN_THRUSTER_CAN_ID, 0x51, 0x43); break; 
            case 2: request_thruster_status(MAIN_THRUSTER_CAN_ID, 0x51, 0x50); break; 
            case 3: request_thruster_status(MAIN_THRUSTER_CAN_ID, 0x45, 0x46); break; 
        }

        // 2. 辅推指令以 100ms 周期下发即可（无需轮询反馈）
        if (ticks % 5 == 0) {
            send_aux_control_commands();
        }

        publish_main_thruster_status();
        publish_aux_thrusters_status();
    }

    void publish_main_thruster_status() {
        hal::msg::HalMainthruster main_msg;
        main_msg.timestamp = this->get_clock()->now().nanoseconds();
        main_msg.rpm = real_main_rpm_.load();
        main_msg.current = static_cast<int16_t>(real_main_current_raw_.load() * 100); 
        main_msg.voltage = static_cast<int16_t>(real_main_voltage_.load());
        uint32_t fault_code = real_main_fault_.load();
        main_msg.fault_status = is_estopped_ ? 0xFF : static_cast<uint8_t>(fault_code & 0xFF);
        pub_main_status_->publish(main_msg);
    }

    void publish_aux_thrusters_status() {
        hal::msg::HalAuxithruster aux_msg;
        aux_msg.timestamp = this->get_clock()->now().nanoseconds();
        int64_t now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()).count();

        for (int i = 0; i < 5; ++i) {
            int64_t last_seen = last_seen_ms_[i].load();
            bool is_online = (last_seen != 0) && ((now_ms - last_seen) < ONLINE_TIMEOUT_MS);

            aux_msg.rpm[i]     = aux_rpm_[i].load();
            aux_msg.current[i] = static_cast<int16_t>(std::round(aux_current_[i].load() * 100.0f)); 
            aux_msg.voltage[i] = static_cast<int16_t>(aux_voltage_[i].load());
            aux_msg.temp[i]    = static_cast<uint16_t>(aux_temp_[i].load());

            uint8_t fault_type = aux_fault_[i].load();
            if (is_estopped_) fault_type = 0xFF; 
            else if (is_emergency_ascending_) fault_type = 0xFD; 
            else if (!is_online) fault_type = 0xFE; 
            
            aux_msg.fault_status[i] = fault_type;
        }
        pub_aux_status_->publish(aux_msg);
    }

    void request_thruster_status(uint32_t target_can_id, uint8_t cmd_byte1, uint8_t cmd_byte2) {
        int fd;
        {
            std::lock_guard<std::mutex> lock(can_socket_mutex_);
            fd = can_socket_.load();
        }
        if (fd < 0) return;
        
        struct can_frame frame;
        // 主推（推进器1）这里直接使用实际 CAN ID 0x301
        frame.can_id = target_can_id; 
        frame.can_dlc = 4; 
        frame.data[0] = cmd_byte1; frame.data[1] = cmd_byte2; frame.data[2] = 0x00; frame.data[3] = 0x00; 
        
        ssize_t bytes_written = write(fd, &frame, sizeof(struct can_frame));
        if (bytes_written < 0 && (errno == ENETDOWN || errno == ENODEV || errno == EBADF)) {
            hardware_api_close_can();
        }
    }

    bool interruptible_sleep(int milliseconds) {
        int elapsed = 0;
        while (elapsed < milliseconds) {
            if (is_estopped_ || !keep_running_ || !is_testing_) return false; 
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            elapsed += 50;
        }
        return true;
    }

    bool hardware_api_init_can(const std::string& can_iface = "can3") {
        {
            std::lock_guard<std::mutex> lock(can_socket_mutex_);
            if (can_socket_.load() >= 0) return true;
        }

        int fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
        if (fd < 0) return false;

        struct ifreq ifr;
        std::strncpy(ifr.ifr_name, can_iface.c_str(), IFNAMSIZ - 1);
        ifr.ifr_name[IFNAMSIZ - 1] = '\0';
        if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) { close(fd); return false; }

        struct timeval tv;
        tv.tv_sec = 0; tv.tv_usec = 50000;
        setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        struct sockaddr_can addr;
        addr.can_family = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;

        if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            close(fd); return false;
        }

        {
            std::lock_guard<std::mutex> lock(can_socket_mutex_);
            can_socket_.store(fd);
        }
        RCLCPP_INFO(get_logger(), "✅ SocketCAN 接口 %s 初始化成功！", can_iface.c_str());
        return true;
    }

    void hardware_api_close_can() {
        int fd;
        {
            std::lock_guard<std::mutex> lock(can_socket_mutex_);
            fd = can_socket_.exchange(-1);
        }
        if (fd >= 0) { close(fd); }
    }
};

int main(int argc, char ** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<HalThrusterNode>("hal_thruster_node");
    rclcpp::spin(node->get_node_base_interface());
    rclcpp::shutdown();
    return 0;
}