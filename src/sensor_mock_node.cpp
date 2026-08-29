/**
 * @file sensor_mock_node.cpp
 * @brief 传感器模拟节点 —— 在无硬件时发布模拟数据供测试
 *
 * 发布话题:
 *   /hal/depthsensor  (HalDepthsensor)  50Hz  深度 ~5m ± 小幅波动
 *   /hal/inertialnavi  (HalInertialnavi) 50Hz  姿态 yaw/pitch/roll
 *   /hal/dvl           (HalDvl)          50Hz  体坐标系速度 (~0)
 *
 * 设计: LifecycleNode, 与现有 HAL 节点风格一致.
 *       启动后自动配置+激活 (通过 launch 文件).
 */

 #include <chrono>
 #include <cmath>
 #include <memory>
 #include <random>
 
 #include "rclcpp/rclcpp.hpp"
 #include "rclcpp_lifecycle/lifecycle_node.hpp"
 #include "rclcpp_lifecycle/lifecycle_publisher.hpp"
 #include "std_msgs/msg/float64_multi_array.hpp"
 
 #include "hal/msg/hal_depthsensor.hpp"
 #include "hal/msg/hal_inertialnavi.hpp"
 #include "hal/msg/hal_dvl.hpp"
 
 using CallbackReturn =
     rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;
 
 class SensorMockNode : public rclcpp_lifecycle::LifecycleNode
 {
 public:
     SensorMockNode()
     : LifecycleNode("sensor_mock_node"), gen_(rd_()) {}
 
     // ==================== 生命周期 ====================
 
     CallbackReturn on_configure(const rclcpp_lifecycle::State &) override
     {
         RCLCPP_INFO(get_logger(), "[MOCK] on_configure");
 
         this->declare_parameter("publish_rate_hz", 50.0);
         this->declare_parameter("base_depth", 5.0);       // 基准深度 [m]
         this->declare_parameter("depth_noise", 0.05);      // 深度噪声(1σ) [m]
         this->declare_parameter("base_yaw", 0.0);          // 基准偏航 [°]
         this->declare_parameter("base_pitch", 0.0);
         this->declare_parameter("base_roll", 0.0);
 
         depth_pub_ = this->create_publisher<hal::msg::HalDepthsensor>(
             "/hal/depthsensor", 10);
         ins_pub_ = this->create_publisher<hal::msg::HalInertialnavi>(
             "/hal/inertialnavi", 10);
         dvl_pub_ = this->create_publisher<hal::msg::HalDvl>(
             "/hal/dvl", 10);
 
         return CallbackReturn::SUCCESS;
     }
 
     CallbackReturn on_activate(const rclcpp_lifecycle::State &) override
     {
         RCLCPP_INFO(get_logger(), "[MOCK] on_activate");
 
         depth_pub_->on_activate();
         ins_pub_->on_activate();
         dvl_pub_->on_activate();
 
         double rate = this->get_parameter("publish_rate_hz").as_double();
         base_depth_  = this->get_parameter("base_depth").as_double();
         depth_noise_ = this->get_parameter("depth_noise").as_double();
         base_yaw_    = this->get_parameter("base_yaw").as_double();
         base_pitch_  = this->get_parameter("base_pitch").as_double();
         base_roll_   = this->get_parameter("base_roll").as_double();
 
         int period_ms = static_cast<int>(1000.0 / rate);
         timer_ = this->create_wall_timer(
             std::chrono::milliseconds(period_ms),
             std::bind(&SensorMockNode::publish_all, this));
 
         RCLCPP_INFO(get_logger(),
             "[MOCK] 已激活, %dHz | depth=%.1f±%.2fm yaw=%.1f°",
             (int)rate, base_depth_, depth_noise_, base_yaw_);
         return CallbackReturn::SUCCESS;
     }
 
     CallbackReturn on_deactivate(const rclcpp_lifecycle::State &) override
     {
         timer_->cancel();
         depth_pub_->on_deactivate();
         ins_pub_->on_deactivate();
         dvl_pub_->on_deactivate();
         return CallbackReturn::SUCCESS;
     }
 
     CallbackReturn on_cleanup(const rclcpp_lifecycle::State &) override
     {
         depth_pub_.reset();
         ins_pub_.reset();
         dvl_pub_.reset();
         return CallbackReturn::SUCCESS;
     }
 
     CallbackReturn on_shutdown(const rclcpp_lifecycle::State &) override
     {
         timer_->cancel();
         return CallbackReturn::SUCCESS;
     }
 
 private:
     void publish_all()
     {
         auto now = this->now();
 
         // --- 深度传感器 ---
         auto depth_msg = hal::msg::HalDepthsensor();
         float noise = depth_dist_(gen_);
         depth_msg.depth_avg = static_cast<float>(base_depth_ + noise);
         depth_msg.depth_1   = depth_msg.depth_avg - 0.02f;
         depth_msg.depth_2   = depth_msg.depth_avg + 0.02f;
         depth_msg.temp_1    = 20;
         depth_msg.temp_2    = 20;
         depth_msg.connection_status = 1;  // 正常
         depth_pub_->publish(depth_msg);
 
         // --- 惯性导航 ---
         auto ins_msg = hal::msg::HalInertialnavi();
         ins_msg.yaw   = static_cast<float>(base_yaw_);
         ins_msg.pitch = static_cast<float>(base_pitch_);
         ins_msg.roll  = static_cast<float>(base_roll_);
         ins_msg.latitude   = 30.0f;
         ins_msg.longitude  = 120.0f;
         ins_msg.east_velocity  = 0.0f;
         ins_msg.north_velocity = 0.0f;
         ins_msg.sky_velocity   = 0.0f;
         ins_msg.connection_status = 1;
         ins_pub_->publish(ins_msg);
 
         // --- DVL速度 ---
         auto dvl_msg = hal::msg::HalDvl();
         dvl_msg.velocity_x = 0.0f;
         dvl_msg.velocity_y = 0.0f;
         dvl_msg.velocity_z = 0.0f;
         dvl_msg.connection_status = 1;
         dvl_pub_->publish(dvl_msg);
     }
 
     // 成员
     rclcpp_lifecycle::LifecyclePublisher<hal::msg::HalDepthsensor>::SharedPtr
         depth_pub_;
     rclcpp_lifecycle::LifecyclePublisher<hal::msg::HalInertialnavi>::SharedPtr
         ins_pub_;
     rclcpp_lifecycle::LifecyclePublisher<hal::msg::HalDvl>::SharedPtr
         dvl_pub_;
     rclcpp::TimerBase::SharedPtr timer_;
 
     double base_depth_  = 5.0;
     double depth_noise_ = 0.05;
     double base_yaw_    = 0.0;
     double base_pitch_  = 0.0;
     double base_roll_   = 0.0;
 
     std::random_device rd_;
     std::mt19937 gen_;
     std::normal_distribution<float> depth_dist_{0.0f, 0.05f};
 };
 
 int main(int argc, char **argv)
 {
     rclcpp::init(argc, argv);
     auto node = std::make_shared<SensorMockNode>();
     rclcpp::executors::SingleThreadedExecutor exec;
     exec.add_node(node->get_node_base_interface());
     exec.spin();
     rclcpp::shutdown();
     return 0;
 }
 