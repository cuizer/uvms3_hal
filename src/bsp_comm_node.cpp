#include <rclcpp/rclcpp.hpp>
#include <rclcpp_lifecycle/lifecycle_node.hpp>

// 【已修改】引用统一后的头文件
#include "hal/msg/hal_inertialnavi.hpp"
#include "hal/msg/hal_dvl.hpp"
#include "hal/msg/hal_depthsensor.hpp"
#include "hal/msg/hal_battery.hpp"

#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

#include <thread>
#include <cstring>
#include <vector>
#include <optional>

using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

class BspCommNode : public rclcpp_lifecycle::LifecycleNode
{
public:
    BspCommNode(): LifecycleNode("bsp_comm_node")
    {
        this->declare_parameter<std::string>("udp_ip", "127.0.0.1");
        this->declare_parameter<int>("udp_port", 5000);
    }

    // ================= 生命周期 =================
    CallbackReturn on_configure(const rclcpp_lifecycle::State &)
    {
        auto qos = rclcpp::QoS(10);

        // 【已修改】统一消息类型名与全局话题路径
        inertial_sub_ = this->create_subscription<hal::msg::HalInertialnavi>(
            "/hal/inertialnavi", qos, std::bind(&BspCommNode::inertial_callback, this, std::placeholders::_1));
        
        dvl_sub_ = this->create_subscription<hal::msg::HalDvl>(
            "/hal/dvl", qos, std::bind(&BspCommNode::dvl_callback, this, std::placeholders::_1));
        
        depthsensor_sub_ = this->create_subscription<hal::msg::HalDepthsensor>(
            "/hal/depthsensor", qos, std::bind(&BspCommNode::depthsensor_callback, this, std::placeholders::_1));
        
        battery_sub_ = this->create_subscription<hal::msg::HalBattery>(
            "/hal/battery", qos, std::bind(&BspCommNode::battery_callback, this, std::placeholders::_1));

        udp_ip_ = this->get_parameter("udp_ip").as_string();
        udp_port_ = this->get_parameter("udp_port").as_int();

        // 创建 UDP socket
        sock_ = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock_ < 0) {
            RCLCPP_ERROR(get_logger(), "Socket create failed");
            return CallbackReturn::FAILURE;
        }

        memset(&target_addr_, 0, sizeof(target_addr_));
        target_addr_.sin_family = AF_INET;
        target_addr_.sin_port = htons(udp_port_);
        inet_pton(AF_INET, udp_ip_.c_str(), &target_addr_.sin_addr);

        timer_ = this->create_wall_timer(std::chrono::milliseconds(20), std::bind(&BspCommNode::udp_send, this));

        RCLCPP_INFO(get_logger(), "BSP通讯节点配置完成，目标地址: %s:%d", udp_ip_.c_str(), udp_port_);
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_activate(const rclcpp_lifecycle::State &)
    {
        active_ = true;
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_deactivate(const rclcpp_lifecycle::State &)
    {
        active_ = false;
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_cleanup(const rclcpp_lifecycle::State &)
    {
        inertial_sub_.reset();
        dvl_sub_.reset();
        depthsensor_sub_.reset();
        battery_sub_.reset();
        timer_.reset();

        if (sock_ >= 0) {
            close(sock_);
            sock_ = -1;
        }
        return CallbackReturn::SUCCESS;
    }

private:
    // ================= 回调函数 =================
    void inertial_callback(const hal::msg::HalInertialnavi::SharedPtr msg)
    {
        if (!active_) return;
        inertial_data_ = *msg;
    }

    void dvl_callback(const hal::msg::HalDvl::SharedPtr msg)
    {
        if (!active_) return;
        dvl_data_ = *msg;
    }

    void depthsensor_callback(const hal::msg::HalDepthsensor::SharedPtr msg)
    {
        if (!active_) return;
        depthsensor_data_ = *msg;
    }

    void battery_callback(const hal::msg::HalBattery::SharedPtr msg)
    {
        if (!active_) return;
        battery_data_ = *msg;
    }

    // ================= 打包函数 (内存拷贝逻辑) =================
    std::vector<uint8_t> pack_inertial(const hal::msg::HalInertialnavi & msg)
    {
        std::vector<uint8_t> buf(sizeof(int64_t) + 8 * sizeof(float));
        uint8_t* p = buf.data();
        memcpy(p, &msg.timestamp, sizeof(int64_t)); p += sizeof(int64_t);
        memcpy(p, &msg.yaw, sizeof(float)); p += sizeof(float);
        memcpy(p, &msg.pitch, sizeof(float)); p += sizeof(float);
        memcpy(p, &msg.roll, sizeof(float)); p += sizeof(float);
        memcpy(p, &msg.latitude, sizeof(float)); p += sizeof(float);
        memcpy(p, &msg.longitude, sizeof(float)); p += sizeof(float);
        memcpy(p, &msg.east_velocity, sizeof(float)); p += sizeof(float);
        memcpy(p, &msg.north_velocity, sizeof(float)); p += sizeof(float);
        memcpy(p, &msg.sky_velocity, sizeof(float));
        return buf;
    }

    std::vector<uint8_t> pack_dvl(const hal::msg::HalDvl & msg)
    {
        std::vector<uint8_t> buf(sizeof(int64_t) + 3 * sizeof(float));
        uint8_t* p = buf.data();
        memcpy(p, &msg.timestamp, sizeof(int64_t)); p += sizeof(int64_t);
        memcpy(p, &msg.velocity_x, sizeof(float)); p += sizeof(float);
        memcpy(p, &msg.velocity_y, sizeof(float)); p += sizeof(float);
        memcpy(p, &msg.velocity_z, sizeof(float));
        return buf;
    }

    std::vector<uint8_t> pack_depthsensor(const hal::msg::HalDepthsensor & msg)
    {
        std::vector<uint8_t> buf(sizeof(int64_t) + 2 * sizeof(float) + 2 * sizeof(uint16_t) + sizeof(float));
        uint8_t* p = buf.data();
        memcpy(p, &msg.timestamp, sizeof(int64_t)); p += sizeof(int64_t);
        memcpy(p, &msg.depth_1, sizeof(float)); p += sizeof(float);
        memcpy(p, &msg.temp_1, sizeof(uint16_t)); p += sizeof(uint16_t);
        memcpy(p, &msg.depth_2, sizeof(float)); p += sizeof(float);
        memcpy(p, &msg.temp_2, sizeof(uint16_t)); p += sizeof(uint16_t);
        memcpy(p, &msg.depth_avg, sizeof(float));
        return buf;
    }

    std::vector<uint8_t> pack_battery(const hal::msg::HalBattery & msg)
    {
        // 计算长度：timestamp(8)+status(1*2)+voltage(2*2)+current(2*2)+cycle(2*2)+temp(2*2)+remain(2*2)+total(2*2)+switch(1*3)
        std::vector<uint8_t> buf(sizeof(int64_t) + 2 + 4 + 4 + 4 + 4 + 4 + 4 + 3);
        uint8_t* p = buf.data();
        memcpy(p, &msg.timestamp, sizeof(int64_t)); p += sizeof(int64_t);
        memcpy(p, &msg.battery_status_48v, 1); p += 1;
        memcpy(p, &msg.battery_status_72v, 1); p += 1;
        memcpy(p, &msg.battery_voltage_48v, 2); p += 2;
        memcpy(p, &msg.battery_voltage_72v, 2); p += 2;
        memcpy(p, &msg.battery_current_48v, 2); p += 2;
        memcpy(p, &msg.battery_current_72v, 2); p += 2;
        memcpy(p, &msg.cycle_count_48v, 2); p += 2;
        memcpy(p, &msg.cycle_count_72v, 2); p += 2;
        memcpy(p, &msg.battery_temperature_48v, 2); p += 2;
        memcpy(p, &msg.battery_temperature_72v, 2); p += 2;
        memcpy(p, &msg.remain_capacity_48v, 2); p += 2;
        memcpy(p, &msg.remain_capacity_72v, 2); p += 2;
        memcpy(p, &msg.total_capacity_48v, 2); p += 2;
        memcpy(p, &msg.total_capacity_72v, 2); p += 2;
        memcpy(p, &msg.switch_state_12v, 1); p += 1;
        memcpy(p, &msg.switch_state_24v, 1); p += 1;
        memcpy(p, &msg.switch_state_72v, 1);
        return buf;
    }

    // ================= 打印与打包逻辑 =================
    void print_inertial(const hal::msg::HalInertialnavi & msg) {
        RCLCPP_INFO(this->get_logger(), "[Inertial] ts: %ld | yaw: %.2f", msg.timestamp, msg.yaw);
    }

    void print_dvl(const hal::msg::HalDvl & msg) {
        RCLCPP_INFO(this->get_logger(), "[Dvl] ts: %ld | vx: %.2f", msg.timestamp, msg.velocity_x);
    }

    void print_depthsensor(const hal::msg::HalDepthsensor & msg) {
        RCLCPP_INFO(this->get_logger(), "[Depth] ts: %ld | avg: %.3f", msg.timestamp, msg.depth_avg);
    }

    void print_battery(const hal::msg::HalBattery & msg) {
        RCLCPP_INFO(this->get_logger(), "[Battery] ts: %ld | 48V: %.1fV", msg.timestamp, msg.battery_voltage_48v * 0.1);
    }

    std::vector<uint8_t> build_packet(uint8_t msg_id, const std::vector<uint8_t>& payload)
    {
        uint16_t header = 0x55AA;
        uint16_t len = static_cast<uint16_t>(payload.size());
        std::vector<uint8_t> packet(sizeof(header) + 1 + sizeof(len) + len);
        uint8_t* p = packet.data();
        memcpy(p, &header, 2); p += 2;
        memcpy(p, &msg_id, 1); p += 1;
        memcpy(p, &len, 2); p += 2;
        memcpy(p, payload.data(), len);
        return packet;
    }

    // ================= UDP 发送 =================
    void udp_send()
    {
        if (!active_) return;

        static int count = 0;
        bool do_print = (++count % 50 == 0);

        // ---------- Inertialnavi (ID: 0x01) ----------
        if (inertial_data_.has_value()) {
            const auto & msg = inertial_data_.value();
            if (do_print) print_inertial(msg);
            auto packet = build_packet(0x01, pack_inertial(msg));
            send_to_udp(packet);
        }

        // ---------- DVL (ID: 0x02) ----------
        if (dvl_data_.has_value()) {
            const auto & msg = dvl_data_.value();
            if (do_print) print_dvl(msg);
            auto packet = build_packet(0x02, pack_dvl(msg));
            send_to_udp(packet);
        }

        // ---------- Depthsensor (ID: 0x03) ----------
        if (depthsensor_data_.has_value()) {
            const auto & msg = depthsensor_data_.value();
            if (do_print) print_depthsensor(msg);
            auto packet = build_packet(0x03, pack_depthsensor(msg));
            send_to_udp(packet);
        }

        // ---------- Battery (ID: 0x04) ----------
        if (battery_data_.has_value()) {
            const auto & msg = battery_data_.value();
            if (do_print) print_battery(msg);
            auto packet = build_packet(0x04, pack_battery(msg)); // 【修正】ID改为0x04
            send_to_udp(packet);
        }
    }

    void send_to_udp(const std::vector<uint8_t>& packet) {
        sendto(sock_, packet.data(), packet.size(), 0, 
               reinterpret_cast<struct sockaddr*>(&target_addr_), sizeof(target_addr_));
    }

private:
    // 【已修改】成员变量类型同步
    rclcpp::Subscription<hal::msg::HalInertialnavi>::SharedPtr inertial_sub_;
    rclcpp::Subscription<hal::msg::HalDvl>::SharedPtr dvl_sub_;
    rclcpp::Subscription<hal::msg::HalDepthsensor>::SharedPtr depthsensor_sub_;
    rclcpp::Subscription<hal::msg::HalBattery>::SharedPtr battery_sub_;
    rclcpp::TimerBase::SharedPtr timer_;

    std::optional<hal::msg::HalInertialnavi> inertial_data_;
    std::optional<hal::msg::HalDvl> dvl_data_;
    std::optional<hal::msg::HalDepthsensor> depthsensor_data_;
    std::optional<hal::msg::HalBattery> battery_data_;

    int sock_{-1};
    std::string udp_ip_;
    int udp_port_;
    struct sockaddr_in target_addr_;

    bool active_{false};
};

int main(int argc, char **argv)
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<BspCommNode>();
    rclcpp::spin(node->get_node_base_interface());
    rclcpp::shutdown();
    return 0;
}