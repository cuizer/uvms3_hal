#include <chrono>
#include <cmath>
#include <memory>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/float64_multi_array.hpp"

using namespace std::chrono_literals;

class TailServoTestNode : public rclcpp::Node {
public:
    TailServoTestNode() : Node("tail_servo_test_node"), start_time_(this->get_clock()->now()) {
        // 创建发布者，对应主节点中的 /hal/servo/tail_cmd 话题
        publisher_ = this->create_publisher<std_msgs::msg::Float64MultiArray>("/hal/servo/tail_cmd", 10);
        
        // 50ms 定时器 (20Hz 的控制频率)
        timer_ = this->create_wall_timer(
            50ms, std::bind(&TailServoTestNode::timer_callback, this));
            
        RCLCPP_INFO(this->get_logger(), "尾部舵机测试节点已启动！");
        RCLCPP_INFO(this->get_logger(), "正在发送正弦波角度指令 [-30° 到 30°]...");
    }

private:
    void timer_callback() {
        auto msg = std_msgs::msg::Float64MultiArray();
        
        // 计算运行时长 (秒)
        auto now = this->get_clock()->now();
        double t = (now - start_time_).seconds();
        
        // 使用正弦函数生成平滑角度
        // 参数说明：30.0 是振幅（±30度），周期设为 4 秒 (频率 = 2*PI / 4.0)
        double period = 4.0;
        double angular_freq = 2.0 * M_PI / period;
        double angle = 30.0 * std::sin(angular_freq * t);
        
        // 根据主节点代码，尾部有 4 个舵机 (ID: 0~3)
        msg.data.resize(4);
        msg.data[0] = angle;      // 舵机 0
        msg.data[1] = -angle;     // 舵机 1 (负号使其做反向对称运动，方便观察)
        msg.data[2] = angle;      // 舵机 2
        msg.data[3] = -angle;     // 舵机 3
        
        // 降低终端打印频率（约每秒打印1次），防止刷屏
        static int print_count = 0;
        if (print_count++ % 20 == 0) {
            RCLCPP_INFO(this->get_logger(), "当前下发角度: %.2f °", angle);
        }
        
        publisher_->publish(msg);
    }

    rclcpp::Publisher<std_msgs::msg::Float64MultiArray>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
    rclcpp::Time start_time_;
};

int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<TailServoTestNode>());
    rclcpp::shutdown();
    return 0;
}