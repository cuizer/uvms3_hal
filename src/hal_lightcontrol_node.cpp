#include <memory>
#include <thread>
#include <atomic>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <string>
#include <fcntl.h>   // Linux 文件控制
#include <unistd.h>  // Linux 系统调用

#include "rclcpp/rclcpp.hpp"
#include "rclcpp_lifecycle/lifecycle_node.hpp"
#include "hal/msg/hal_light_control.hpp"

using namespace std::chrono_literals;
using CallbackReturn = rclcpp_lifecycle::node_interfaces::LifecycleNodeInterface::CallbackReturn;

class HalLightSoftwarePwmNode : public rclcpp_lifecycle::LifecycleNode
{
public:
    explicit HalLightSoftwarePwmNode(const rclcpp::NodeOptions & options)
    : rclcpp_lifecycle::LifecycleNode("hal_light_sw_pwm_node", options),
      duty_cycle_(0.0f),
      running_(false),
      gpio_fd_(-1)
    {
        // ������ 核心改变：不再请求芯片，而是直接指向米文官方留的后门路径
        this->declare_parameter<std::string>("gpio_path", "/dev/gpio/do0/value"); 
        this->declare_parameter<bool>("active_low", false); 
        this->declare_parameter<int>("pwm_freq_hz", 50);

        RCLCPP_INFO(this->get_logger(), ">>> [HAL Sysfs版控制大脑] 已构建，准备接入系统后门 <<<");
    }

    ~HalLightSoftwarePwmNode()
    {
        stop_pwm_thread();
        close_gpio();
    }

protected:
    CallbackReturn on_configure(const rclcpp_lifecycle::State &) override
    {
        this->get_parameter("gpio_path", gpio_path_);
        this->get_parameter("active_low", active_low_);
        
        int freq_hz = 50;
        this->get_parameter("pwm_freq_hz", freq_hz);
        pwm_period_us_ = 1000000 / freq_hz; 

        on_str_ = active_low_ ? "0" : "1";
        off_str_ = active_low_ ? "1" : "0";

        RCLCPP_INFO(this->get_logger(), "--- [配置中] 顺水推舟，尝试接入路径: %s ---", gpio_path_.c_str());

        // 使用最高效的底层文件 I/O 打开设备
        gpio_fd_ = open(gpio_path_.c_str(), O_WRONLY);
        if (gpio_fd_ < 0) {
            RCLCPP_FATAL(this->get_logger(), "❌ 无法打开 %s！请确认文件路径是否存在！", gpio_path_.c_str());
            return CallbackReturn::FAILURE;
        }

        // 初始状态强制关灯保护
        pwrite(gpio_fd_, off_str_.c_str(), 1, 0);

        // 订阅 0~255 的调光控制话题
        light_cmd_sub_ = this->create_subscription<hal::msg::HalLightControl>(
            "/hal/lightcontrol", 10,
            std::bind(&HalLightSoftwarePwmNode::pwm_cmd_callback, this, std::placeholders::_1)
        );

        RCLCPP_INFO(this->get_logger(), "������ [配置成功] 成功接管系统底层文件。初始状态已断电保护。");
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_activate(const rclcpp_lifecycle::State & state) override
    {
        LifecycleNode::on_activate(state);
        
        duty_cycle_ = 0.0f; // 激活时默认全灭
        running_ = true;
        pwm_thread_ = std::thread(&HalLightSoftwarePwmNode::software_pwm_loop, this);
        
        RCLCPP_WARN(this->get_logger(), "������������������ [节点已激活] Sysfs 架构高频 PWM 线程已拉起，随时准备调光！");
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_deactivate(const rclcpp_lifecycle::State & state) override
    {
        LifecycleNode::on_deactivate(state);
        RCLCPP_WARN(this->get_logger(), "������ [节点已钝化] 正在掐死高频线程并执行物理关灯...");
        stop_pwm_thread();
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_cleanup(const rclcpp_lifecycle::State &) override
    {
        close_gpio();
        light_cmd_sub_.reset();
        return CallbackReturn::SUCCESS;
    }

    CallbackReturn on_shutdown(const rclcpp_lifecycle::State &) override
    {
        stop_pwm_thread();
        close_gpio();
        return CallbackReturn::SUCCESS;
    }

private:
    void pwm_cmd_callback(const hal::msg::HalLightControl::SharedPtr msg)
    {
        if (this->get_current_state().id() != 3) return; // 仅在 ACTIVE 处理

        float target_duty = msg->light_coeff / 255.0f;
        duty_cycle_ = std::clamp(target_duty, 0.0f, 1.0f);
        
        RCLCPP_INFO(this->get_logger(), "【大脑指令】接收调光信号 (RAW: %d) ➔ 目标亮度: %.0f%%", 
                    msg->light_coeff, duty_cycle_.load() * 100.0);
    }

    void software_pwm_loop()
    {
        while (running_) {
            float current_duty = duty_cycle_.load();

            if (current_duty <= 0.01f) {
                pwrite(gpio_fd_, off_str_.c_str(), 1, 0); 
                std::this_thread::sleep_for(std::chrono::microseconds(pwm_period_us_));
            } 
            else if (current_duty >= 0.99f) {
                pwrite(gpio_fd_, on_str_.c_str(), 1, 0); 
                std::this_thread::sleep_for(std::chrono::microseconds(pwm_period_us_));
            } 
            else {
                int on_time_us = static_cast<int>(pwm_period_us_ * current_duty);
                int off_time_us = pwm_period_us_ - on_time_us;

                pwrite(gpio_fd_, on_str_.c_str(), 1, 0);
                std::this_thread::sleep_for(std::chrono::microseconds(on_time_us));

                pwrite(gpio_fd_, off_str_.c_str(), 1, 0);
                std::this_thread::sleep_for(std::chrono::microseconds(off_time_us));
            }
        }
    }

    void stop_pwm_thread()
    {
        running_ = false;
        if (pwm_thread_.joinable()) {
            pwm_thread_.join();
        }
        if (gpio_fd_ >= 0) {
            pwrite(gpio_fd_, off_str_.c_str(), 1, 0); // 强行补发关灯指令
        }
    }

    void close_gpio()
    {
        if (gpio_fd_ >= 0) {
            close(gpio_fd_);
            gpio_fd_ = -1;
        }
    }

    std::string gpio_path_;
    bool active_low_;
    int pwm_period_us_;
    std::string on_str_;
    std::string off_str_;
    
    int gpio_fd_;
    
    std::atomic<float> duty_cycle_; 
    std::atomic<bool> running_;
    std::thread pwm_thread_;

    rclcpp::Subscription<hal::msg::HalLightControl>::SharedPtr light_cmd_sub_;
};

int main(int argc, char ** argv)
{
    rclcpp::init(argc, argv);
    rclcpp::NodeOptions options;
    auto node = std::make_shared<HalLightSoftwarePwmNode>(options);
    
    rclcpp::executors::SingleThreadedExecutor exe;
    exe.add_node(node->get_node_base_interface());
    exe.spin();
    
    rclcpp::shutdown();
    return 0;
}