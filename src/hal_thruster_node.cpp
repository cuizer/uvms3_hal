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

        pub_main_status_ = this->create_publisher<hal::msg::HalMainthrusterMsg>(
            "hal_mainthruster_msg", 10);
            
        pub_aux_status_ = this->create_publisher<hal::msg::HalAuxithrusterMsg>(
            "hal_auxithruster_msg", 10);

        srv_control_ = this->create_service<hal::srv::HalThrustercontrolSrv>(
            "hal_thrustercontrol_srv",
            std::bind(&HalThrusterNode::control_srv_callback, this, _1, _2));

        // 🚨 新增：注册独立的急停服务
        srv_estop_ = this->create_service<std_srvs::srv::SetBool>(
            "hal_thruster_estop",
            std::bind(&HalThrusterNode::estop_callback, this, _1, _2));

        sub_cmd_ = this->create_subscription<std_msgs::msg::Float64MultiArray>(
            "/hal/thruster/cmd", 10,
            std::bind(&HalThrusterNode::cmd_callback, this, _1));

        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(20),
            std::bind(&HalThrusterNode::timer_publish_status_callback, this));

        hardware_api_init_can("can0");
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_activate(const rclcpp_lifecycle::State & state) override {
        RCLCPP_INFO(get_logger(), "推进器节点已激活。");
        pub_main_status_->on_activate();
        pub_aux_status_->on_activate();
        is_testing_ = false;
        is_estopped_ = false; // 🚨 新增：激活时默认解除急停
        LifecycleNode::on_activate(state);
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_deactivate(const rclcpp_lifecycle::State & state) override {
        pub_main_status_->on_deactivate();
        pub_aux_status_->on_deactivate();
        stop_all_thrusters();
        LifecycleNode::on_deactivate(state);
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_cleanup(const rclcpp_lifecycle::State &) override {
        pub_main_status_.reset(); pub_aux_status_.reset();
        srv_control_.reset(); srv_estop_.reset(); sub_cmd_.reset(); timer_.reset();
        
        // 🚨 新增：释放底层硬件资源
        hardware_api_close_can();

        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_shutdown(const rclcpp_lifecycle::State &) override {
        stop_all_thrusters();
        // 🚨 新增：节点彻底退出前关闭 CAN 通道
        hardware_api_close_can();

        return CallbackReturn::SUCCESS;
    }

private:
    std::atomic<bool> is_estopped_{false}; // 🚨 新增：急停标志位（线程安全）
    bool is_testing_ = false; 

    int can_socket_ = -1; // SocketCAN 句柄

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
            RCLCPP_INFO(get_logger(), "收到指令 01：开始【主推】正反转各3秒测试...");
            std::thread(&HalThrusterNode::execute_test_sequence, this, true).detach();
            response->success = true; response->message = "主推测试序列已启动";
        } 
        else if (cmd == 0x02) {
            RCLCPP_INFO(get_logger(), "收到指令 02：开始【辅推】正反转各3秒测试...");
            std::thread(&HalThrusterNode::execute_test_sequence, this, false).detach();
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
        if (is_estopped_) return;
        
        // 1. 全部下发正向 10%
        for (uint32_t node_id : test_nodes) {
            set_thruster_rpm_hardware(node_id, test_thrust_pct); 
        }
        std::this_thread::sleep_for(std::chrono::seconds(3));
        
        // 2. 全部下发反向 10%
        if (is_estopped_) return;
        for (uint32_t node_id : test_nodes) {
            set_thruster_rpm_hardware(node_id, -test_thrust_pct); 
        }
        std::this_thread::sleep_for(std::chrono::seconds(3));

        // 3. 全部停机
        for (uint32_t node_id : test_nodes) {
            set_thruster_rpm_hardware(node_id, 0.0);       
        }
        
        is_testing_ = false; 
        if (!is_estopped_) {
            RCLCPP_INFO(get_logger(), "测试序列执行完毕，恢复正常控制。");
        }
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

    void timer_publish_status_callback() {
        if (this->get_current_state().id() != lifecycle_msgs::msg::State::PRIMARY_STATE_ACTIVE) return;

        hal::msg::HalMainthrusterMsg main_msg;
        main_msg.rpm = hardware_read_main_rpm();         
        main_msg.current = hardware_read_main_current(); 
        main_msg.voltage = hardware_read_main_voltage(); 
        main_msg.fault_status = is_estopped_ ? 0xFF : 0x00; // 🚨 可选：急停时上报特定故障码                
        pub_main_status_->publish(main_msg);

        hal::msg::HalAuxithrusterMsg aux_msg;
        for (int i = 0; i < 6; ++i) {
            aux_msg.rpm[i] = hardware_read_aux_rpm(i);         
            aux_msg.current[i] = hardware_read_aux_current(i); 
            aux_msg.voltage[i] = hardware_read_aux_voltage(i); 
            aux_msg.temp[i] = hardware_read_aux_temp(i);       
            aux_msg.esc_status[i] = 0x01;                      
            aux_msg.fault_status[i] = is_estopped_ ? 0xFF : 0x00; // 🚨 可选：急停时上报特定故障码               
        }
        pub_aux_status_->publish(aux_msg);
    }

    // --- 底层硬件接口占位 ---
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
        if (can_socket_ < 0) {
            RCLCPP_WARN_THROTTLE(get_logger(), *get_clock(), 1000, "CAN 未连接，无法发送!");
            return;
        }

        // 1. 推力限幅保护 [cite: 199]
        if (thrust_percentage > 100.0) thrust_percentage = 100.0;
        if (thrust_percentage < -100.0) thrust_percentage = -100.0;

        // 2. 确定 CAN ID [cite: 95, 96],按需修改
                     
        uint32_t can_id = 0x300 + target_node_id;
        // 3. 将百分比转为 32 位整型 [cite: 104]
        int32_t target_thrust = static_cast<int32_t>(thrust_percentage);

        // 4. 组装标准 Linux CAN 帧
        struct can_frame frame;
        frame.can_id = can_id; // 标准11位帧 [cite: 91]
        frame.can_dlc = 8;     // 数据长度 8 字节 [cite: 98]

        frame.data[0] = 0x54; // 'T' [cite: 102]
        frame.data[1] = 0x43; // 'C' [cite: 102]
        frame.data[2] = 0x00; // 预留 [cite: 103]
        frame.data[3] = 0x00; // 数据类型：0x00 表示整型 [cite: 104]

        // 🚨 32 位数据使用大端模式填入 [cite: 105]
        frame.data[4] = (target_thrust >> 24) & 0xFF;
        frame.data[5] = (target_thrust >> 16) & 0xFF;
        frame.data[6] = (target_thrust >> 8) & 0xFF;
        frame.data[7] = target_thrust & 0xFF;

        // 5. 通过 Socket 发送出去
        ssize_t nbytes = write(can_socket_, &frame, sizeof(struct can_frame));
        if (nbytes != sizeof(struct can_frame)) {
            RCLCPP_ERROR_THROTTLE(get_logger(), *get_clock(), 1000, "CAN 数据发送错误!");
        }
    }
    
    int16_t hardware_read_main_rpm() { return 1500; }
    int16_t hardware_read_main_current() { return 500; }
    int16_t hardware_read_main_voltage() { return 4800; }
    
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