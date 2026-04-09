#include <memory>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <atomic> // 🚨 新增：用于线程安全的急停标志位
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <cstring>

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "rclcpp_lifecycle/lifecycle_publisher.hpp"
#include "lifecycle_msgs/msg/state.hpp"
#include "std_msgs/msg/float64_multi_array.hpp" 
#include "std_srvs/srv/set_bool.hpp" // 🚨 新增：急停服务标准类型

#include "hal/msg/hal_mainthruster_msg.hpp"
#include "hal/msg/hal_auxithruster_msg.hpp"
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
        pub_main_status_ = this->create_publisher<hal::msg::HalMainthrusterMsg>("hal_mainthruster_msg", 10);
        pub_aux_status_ = this->create_publisher<hal::msg::HalAuxithrusterMsg>("hal_auxithruster_msg", 10);

        srv_control_ = this->create_service<hal::srv::HalThrustercontrolSrv>(
            "hal_thrustercontrol_srv", std::bind(&HalThrusterNode::control_srv_callback, this, _1, _2));

        srv_estop_ = this->create_service<std_srvs::srv::SetBool>(
            "hal_thruster_estop", std::bind(&HalThrusterNode::estop_callback, this, _1, _2));

        sub_cmd_ = this->create_subscription<std_msgs::msg::Float64MultiArray>(
            "/hal/thruster/cmd", 10, std::bind(&HalThrusterNode::cmd_callback, this, _1));

        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(20), std::bind(&HalThrusterNode::timer_publish_status_callback, this));

        hardware_api_init_can("can0");
        
        // 🚨 删除了这里启动线程的代码！配置阶段只做初始化，不干活。
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_activate(const rclcpp_lifecycle::State & state) override {
        RCLCPP_INFO(get_logger(), "推进器节点已激活。");
        pub_main_status_->on_activate();
        pub_aux_status_->on_activate();
        
        is_testing_ = false;
        is_estopped_ = false; 
        
        // 🚨 开启接收线程 (先确保旧线程关闭，再开新线程)
        keep_running_ = false; // 先发停止信号防御一下
        if (can_rx_thread_.joinable()) can_rx_thread_.join();
        
        keep_running_ = true;  // 赋予通行证
        can_rx_thread_ = std::thread(&HalThrusterNode::can_receive_loop, this);
        
        LifecycleNode::on_activate(state);
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_cleanup(const rclcpp_lifecycle::State &) override {
        pub_main_status_.reset(); pub_aux_status_.reset();
        srv_control_.reset(); srv_estop_.reset(); sub_cmd_.reset(); timer_.reset();
        
        // 🚨 顺序修正：先停线程，再关硬件
        keep_running_ = false; 
        if (can_rx_thread_.joinable()) can_rx_thread_.join();
        if (test_thread_.joinable()) test_thread_.join();
        
        hardware_api_close_can(); // 注意这里原来写了两次，留一次就够了
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_deactivate(const rclcpp_lifecycle::State & state) override {
        pub_main_status_->on_deactivate();
        pub_aux_status_->on_deactivate();
        stop_all_thrusters();
        LifecycleNode::on_deactivate(state);
        // 安全关闭接收线程
        keep_running_ = false;
        if (can_rx_thread_.joinable()) can_rx_thread_.join();
        return CallbackReturn::SUCCESS;
    }

    

    CallbackReturn on_shutdown(const rclcpp_lifecycle::State &) override {
        stop_all_thrusters();
        // 🚨 新增：节点彻底退出前关闭 CAN 通道
        hardware_api_close_can();

        return CallbackReturn::SUCCESS;
    }

private:
    const uint32_t MAIN_THRUSTER_ID = 0x01; // 依据 STM32 代码，Node-ID 设为 1
    std::atomic<bool> is_estopped_{false}; // 🚨 新增：急停标志位（线程安全）
    std::atomic<bool> is_testing_{false}; // 🚨 修改：改为原子变量

    int can_socket_ = -1; // SocketCAN 句柄

    // 🚨 新增：线程管理与停止标志
    std::thread test_thread_;
    std::thread can_rx_thread_;
    std::atomic<bool> keep_running_{false}; // 用于安全退出所有线程

    // 缓存从 CAN 读取的真实硬件状态 (原子操作保证线程安全)
    std::atomic<int32_t> real_main_rpm_{0};
    std::atomic<int32_t> real_main_current_raw_{0}; // 单位: 0.1A 
    std::atomic<int32_t> real_main_voltage_{0};     // 单位: V [cite: 208]
    std::atomic<uint32_t> real_main_fault_{0};
    
    std::shared_ptr<rclcpp_lifecycle::LifecyclePublisher<hal::msg::HalMainthrusterMsg>> pub_main_status_;
    std::shared_ptr<rclcpp_lifecycle::LifecyclePublisher<hal::msg::HalAuxithrusterMsg>> pub_aux_status_;
    rclcpp::Service<hal::srv::HalThrustercontrolSrv>::SharedPtr srv_control_;
    rclcpp::Service<std_srvs::srv::SetBool>::SharedPtr srv_estop_; // 🚨 新增：急停服务指针
    rclcpp::Subscription<std_msgs::msg::Float64MultiArray>::SharedPtr sub_cmd_;
    rclcpp::TimerBase::SharedPtr timer_;

    // 🚨 新增：急停服务回调函数
    void estop_callback(
        const std::shared_ptr<std_srvs::srv::SetBool::Request> request,
        std::shared_ptr<std_srvs::srv::SetBool::Response> response)
    {
        if (request->data) {
            is_estopped_ = true;
            is_testing_ = false;   // 强行打断可能正在进行的测试
            stop_all_thrusters();  // 立即下发停机指令到硬件
            
            RCLCPP_FATAL(get_logger(), "🚨 触发急停！已切断所有控制指令并停止推进器。");
            response->success = true;
            response->message = "急停已激活，推进器已锁定。";
        } else {
            is_estopped_ = false;
            
            RCLCPP_INFO(get_logger(), "✅ 急停已解除，恢复接收控制指令。");
            response->success = true;
            response->message = "急停已解除。";
        }
    }

    void control_srv_callback(
        const std::shared_ptr<hal::srv::HalThrustercontrolSrv::Request> request,
        std::shared_ptr<hal::srv::HalThrustercontrolSrv::Response> response) 
    {
        if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) {
            response->success = false; response->message = "节点未激活，无法执行测试。"; return;
        }

        // 🚨 新增：急停拦截网，拒绝执行测试
        if (is_estopped_) {
            response->success = false; 
            response->message = "当前处于急停状态，拒绝执行测试指令。请先解除急停。"; 
            return;
        }

        if (is_testing_) {
            response->success = false; response->message = "推进器正在测试中，请稍后再试。"; return;
        }

        uint8_t cmd = request->command;
        
        if (cmd == 0x01) {
            // 🚨 确保之前的线程已回收
            if (test_thread_.joinable()) test_thread_.join(); 
            test_thread_ = std::thread(&HalThrusterNode::execute_test_sequence, this, true);
            response->success = true; response->message = "主推测试启动";
        }
        else if (cmd == 0x02) {
            RCLCPP_INFO(get_logger(), "收到指令 02：开始【辅推】正反转各3秒测试...");
            // 🚨 修复：不使用 detach，统一用 test_thread_ 管理
            if (test_thread_.joinable()) test_thread_.join(); 
            test_thread_ = std::thread(&HalThrusterNode::execute_test_sequence, this, false);
            response->success = true; response->message = "辅推测试序列已启动";
        }
        else {
            response->success = false; response->message = "未知指令代码";
        }
    }

    void hardware_api_close_can() {
        if (can_socket_ >= 0) {
            close(can_socket_);
            can_socket_ = -1;
        }
        }

    void execute_test_sequence(bool is_main_thruster) {
        is_testing_ = true; 
        double test_thrust_pct = 10.0; // 10% 推力
        
        // 🚨 新增：用一个数组装载需要测试的 Node ID
        std::vector<uint32_t> test_nodes;
        if (is_main_thruster) {
            test_nodes.push_back(0x0B); // 主推 ID 为 0x0B
        } else {
            // 辅推有 6 个，假设 ID 分别是 0x01 到 0x06
            for (uint32_t i = 1; i <= 6; ++i) {
                test_nodes.push_back(i);
            }
        }
        
        // 🚨 增强：测试序列执行前检查急停
        if (is_estopped_) { is_testing_ = false; return; }
        

        // 1. 正向转动
        for (uint32_t id : test_nodes) set_thruster_rpm_hardware(id, test_thrust_pct);
        
        // 🚨 使用可打断睡眠替代 std::this_thread::sleep_for
        if (!interruptible_sleep(3000)) {
            is_testing_ = false;
            return; // 发生急停或节点退出，直接终止序列
        }

        // 2. 反向转动
        for (uint32_t id : test_nodes) set_thruster_rpm_hardware(id, -test_thrust_pct);
        
        if (!interruptible_sleep(3000)) {
            is_testing_ = false;
            return; 
        }

        // 3. 正常结束测试，发送停机
        for (uint32_t id : test_nodes) set_thruster_rpm_hardware(id, 0.0);
        is_testing_ = false; 
    }
       
    void cmd_callback(const std_msgs::msg::Float64MultiArray::SharedPtr msg) {
        if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) return;
        
        // 🚨 新增：急停拦截网，处于急停状态时直接丢弃控制下发数据
        if (is_estopped_) {
            RCLCPP_DEBUG_THROTTLE(get_logger(), *get_clock(), 1000, "处于急停状态，忽略常规控制指令！");
            return;
        }
        
        if (is_testing_) return; 


        if (msg->data.size() >= 7) {
            // 假设主推 ID 是 0x0B
            set_thruster_rpm_hardware(0x0B, msg->data[0]);  
            
            // 假设 6 个辅推的 ID 分别是 0x01 到 0x06
            for (int i = 1; i < 7; ++i) {
                set_thruster_rpm_hardware(i, msg->data[i]);
            }
        }
    }


    // 🚨 实现 CAN 初始化 (绑定 can0)
    bool hardware_api_init_can(const std::string& can_iface = "can0") {
        // 1. 创建 RAW Socket
        can_socket_ = socket(PF_CAN, SOCK_RAW, CAN_RAW);
        if (can_socket_ < 0) {
            RCLCPP_ERROR(get_logger(), "CAN Socket 创建失败!");
            return false;
        }

        // 2. 获取接口索引 (把 "can0" 转换成系统内部的索引号)
        struct ifreq ifr;
        std::strncpy(ifr.ifr_name, can_iface.c_str(), IFNAMSIZ - 1);
        ifr.ifr_name[IFNAMSIZ - 1] = '\0';
        if (ioctl(can_socket_, SIOCGIFINDEX, &ifr) < 0) {
            RCLCPP_ERROR(get_logger(), "找不到指定的 CAN 接口: %s", can_iface.c_str());
            close(can_socket_);
            can_socket_ = -1;
            return false;
        }

        // 3. 绑定 Socket 到指定的 CAN 接口
        struct sockaddr_can addr;
        addr.can_family = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;
        if (bind(can_socket_, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            RCLCPP_ERROR(get_logger(), "绑定 CAN 接口失败!");
            close(can_socket_);
            can_socket_ = -1;
            return false;
        }

        RCLCPP_INFO(get_logger(), "✅ SocketCAN 接口 %s 初始化成功！", can_iface.c_str());
        return true;
    }

    void stop_all_thrusters() {
        RCLCPP_WARN(get_logger(), "正在向底层硬件下发全推进器停机指令！");
        
        // 发送主推停止指令
        set_thruster_rpm_hardware(0x0B, 0.0); // 停止主推 (ID: 0x0B)
        
        // 未来完善了辅推的 Node ID 逻辑后，这里也要循环发送辅推的停止指令
        // 例如：
        for (int i = 1; i <= 6; ++i) {
             set_thruster_rpm_hardware(i, 0.0); // 停止辅推 (ID: 0x01 到 0x06)
        }
    }
    
    // 🚨 实现 CAN 发送函数
    void set_thruster_rpm_hardware(uint32_t target_node_id, double thrust_percentage) {
        if (can_socket_ < 0) return;
        if (thrust_percentage > 100.0) thrust_percentage = 100.0;
        if (thrust_percentage < -100.0) thrust_percentage = -100.0;

        uint32_t can_id = 0x300 + target_node_id;
        int32_t target_thrust = static_cast<int32_t>(thrust_percentage);
        struct can_frame frame;
        frame.can_id = can_id; 
        frame.can_dlc = 8; // 设置类指令包含 8 字节 [cite: 107]

        frame.data[0] = 0x54; // 'T' [cite: 102]
        frame.data[1] = 0x43; // 'C' [cite: 102]
        frame.data[2] = 0x00; 
        frame.data[3] = 0x00; 

        // 大端模式 [cite: 105]
        frame.data[4] = (target_thrust >> 24) & 0xFF;
        frame.data[5] = (target_thrust >> 16) & 0xFF;
        frame.data[6] = (target_thrust >> 8) & 0xFF;
        frame.data[7] = target_thrust & 0xFF;

        write(can_socket_, &frame, sizeof(struct can_frame));
    }
    void can_receive_loop() {
        struct can_frame frame;
        // 根据手册，推进器返回的 PDO2 发送 ID 为 0x280 + Node_ID [cite: 114]
        uint32_t target_rx_id = 0x280 + MAIN_THRUSTER_ID; 

        while (keep_running_) {
            if (can_socket_ < 0) {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                continue;
            }

            ssize_t nbytes = recv(can_socket_, &frame, sizeof(struct can_frame), MSG_DONTWAIT);
            if (nbytes == sizeof(struct can_frame)) {
                // 判断是否为目标主推的反馈帧
                if (frame.can_id == target_rx_id && frame.can_dlc == 8) {
                    // 解析携带数据：大端模式拼接 32 位整型 [cite: 105]
                    int32_t value = (frame.data[4] << 24) | 
                                    (frame.data[5] << 16) | 
                                    (frame.data[6] << 8)  | 
                                     frame.data[7];

                    // 依据命令字(Byte0, Byte1)区分数据类型
                    if (frame.data[0] == 0x51) {
                        switch (frame.data[1]) {
                            case 0x56: // 'QV' 读取速度 [cite: 201]
                                real_main_rpm_ = value;
                                break;
                            case 0x43: // 'QC' 读取电流 [cite: 204]
                                real_main_current_raw_ = value;
                                break;
                            case 0x50: // 'QP' 读取电压 [cite: 207]
                                real_main_voltage_ = value;
                                break;
                            case 0x54: // 'QT' 读取温度 [cite: 210]
                                // 按需可增加温度缓存
                                break;
                        }
                    } else if (frame.data[0] == 0x45 && frame.data[1] == 0x46) {
                        // 'EF' 错误故障 [cite: 214]
                        real_main_fault_ = static_cast<uint32_t>(value);
                    }
                }
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        }
    }
    // 🚨 新增：可被打断的睡眠函数
    bool interruptible_sleep(int milliseconds) {
        int elapsed = 0;
        while (elapsed < milliseconds) {
            if (is_estopped_ || !keep_running_) return false; // 随时打断
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            elapsed += 50;
        }
        return true;
    }
    
    // 发送参数查询指令 (数据长度为 4 字节) [cite: 107]
    void request_thruster_status(uint32_t target_node_id, uint8_t cmd_byte1, uint8_t cmd_byte2) {
        if (can_socket_ < 0) return;

        struct can_frame frame;
        frame.can_id = 0x300 + target_node_id; 
        frame.can_dlc = 4; // 获取类命令只发送 4 字节 [cite: 107]

        frame.data[0] = cmd_byte1;
        frame.data[1] = cmd_byte2;
        frame.data[2] = 0x00; // 预留 [cite: 103]
        frame.data[3] = 0x00; // 数据类型 [cite: 104]

        write(can_socket_, &frame, sizeof(struct can_frame));
    }

    // 修改你原本的 20ms 定时器回调
    void timer_publish_status_callback() {
        if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) return;

        // 1. 轮询发送查询指令 (交替请求，防止瞬间灌满 CAN 邮箱)
        static int query_step = 0;
        switch (query_step % 4) {
            case 0: request_thruster_status(MAIN_THRUSTER_ID, 0x51, 0x56); break; // 查速度 [cite: 201]
            case 1: request_thruster_status(MAIN_THRUSTER_ID, 0x51, 0x43); break; // 查电流 [cite: 204]
            case 2: request_thruster_status(MAIN_THRUSTER_ID, 0x51, 0x50); break; // 查电压 [cite: 207]
            case 3: request_thruster_status(MAIN_THRUSTER_ID, 0x45, 0x46); break; // 查故障 [cite: 214]
        }
        query_step++;

        // 2. 打包缓存中的状态数据发布出去
        hal::msg::HalMainthrusterMsg main_msg;
        main_msg.rpm = real_main_rpm_.load();
        
        // 电流单位转换：协议返回单位是 0.1A，需依据你们 ROS msg 的定义转换 
        // 假设你们 ROS 里用的是实际 A(安培) 或 mA，这里给出一个通用转算：
        main_msg.current = static_cast<int16_t>(real_main_current_raw_.load() * 100); // 假设消息里需要 mA
        
        main_msg.voltage = static_cast<int16_t>(real_main_voltage_.load());
        
        // 故障码融合急停状态
        uint32_t fault_code = real_main_fault_.load();
        main_msg.fault_status = is_estopped_ ? 0xFF : static_cast<uint8_t>(fault_code & 0xFF);
        
        pub_main_status_->publish(main_msg);
    }

    // --- 主推数据读取接口 ---
    int16_t hardware_read_main_rpm() { 
        // 加上 static_cast 消除从 int32_t 到 int16_t 的编译警告
        return static_cast<int16_t>(real_main_rpm_.load()); 
    }
    
    int16_t hardware_read_main_current() { 
        // 注意变量名：我们之前在接收线程里起的名字是 real_main_current_raw_
        // 大洋协议里电流的单位是 0.1A（比如返回 15 代表 1.5A）
        // 如果你的 ROS 消息里单位也是 0.1A，就直接返回：
        //return static_cast<int16_t>(real_main_current_raw_.load()); 
        
        // 💡 提示：如果你的 ROS 消息里规定电流单位是毫安 (mA)，你要改成：
        return static_cast<int16_t>(real_main_current_raw_.load() * 100);
    }
    
    int16_t hardware_read_main_voltage() { 
        return static_cast<int16_t>(real_main_voltage_.load()); 
    }
    int16_t hardware_read_aux_rpm(int) { return 1000; }
    int16_t hardware_read_aux_current(int) { return 200; }
    int16_t hardware_read_aux_voltage(int) { return 2400; }
    uint16_t hardware_read_aux_temp(int) { return 45; }
};

int main(int argc, char ** argv) {
    rclcpp::init(argc, argv);
    auto node = std::make_shared<HalThrusterNode>("hal_thruster_node");
    rclcpp::spin(node->get_node_base_interface());
    rclcpp::shutdown();
    return 0;
}