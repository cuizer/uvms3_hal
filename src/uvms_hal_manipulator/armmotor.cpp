#include <rclcpp/rclcpp.hpp>

#include "hal/msg/hal_armmotor.hpp"
#include "std_msgs/msg/u_int8.hpp"
#include "trajectory_msgs/msg/joint_trajectory_point.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>

using namespace std::chrono_literals;

class ArmmotorAggregatorNode : public rclcpp::Node
{
public:
    ArmmotorAggregatorNode()
    : Node("armmotor_aggregator_node")
    {
        this->declare_parameter<std::string>("left_arm_topic", "/left_arm/hal/armmotor");
        this->declare_parameter<std::string>("right_arm_topic", "/right_arm/hal/armmotor");
        this->declare_parameter<std::string>("gripper_motor_topic", "/hal/grippermotor");
        this->declare_parameter<std::string>("output_topic", "/hal/armmotor");
        this->declare_parameter<std::string>("arm_control_topic", "/hal/armcontrol");
        this->declare_parameter<std::string>(
            "left_target_topic", "/left_arm/bsp/manipulator/target_joint");
        this->declare_parameter<std::string>(
            "right_target_topic", "/right_arm/bsp/manipulator/target_joint");
        this->declare_parameter<double>("publish_rate_hz", 50.0);
        this->declare_parameter<double>("stale_timeout_sec", 0.5);
        this->declare_parameter<double>("startup_grace_sec", 30.0);

        this->declare_parameter<bool>("enable_csv_logging", false);
        this->declare_parameter<std::string>("csv_log_directory", "armmotor_logs");
        this->declare_parameter<std::string>("csv_log_file_prefix", "armmotor");
        this->declare_parameter<int>("csv_flush_every_n", 50);

        this->declare_parameter<std::vector<int64_t>>("left_motor_ids", {1, 3, 5, 7, 9});
        this->declare_parameter<std::vector<int64_t>>("right_motor_ids", {2, 4, 6, 8, 10});
        this->declare_parameter<int>("joint_motor_count", 10);
        this->declare_parameter<int>("gripper_motor_count", 0);

        // 实机伸出/回收角度必须确认后再启用，默认禁止动作以避免误运动。
        this->declare_parameter<bool>("motion_presets_enabled", false);
        this->declare_parameter<std::vector<double>>(
            "left_extend_positions", {0.0, 0.0, 0.0, 0.0, 0.0});
        this->declare_parameter<std::vector<double>>(
            "right_extend_positions", {0.0, 0.0, 0.0, 0.0, 0.0});
        this->declare_parameter<std::vector<double>>(
            "left_retract_positions", {0.0, 0.0, 0.0, 0.0, 0.0});
        this->declare_parameter<std::vector<double>>(
            "right_retract_positions", {0.0, 0.0, 0.0, 0.0, 0.0});
        this->declare_parameter<double>("extend_duration_sec", 5.0);
        this->declare_parameter<double>("retract_duration_sec", 8.0);

        left_arm_topic_ = this->get_parameter("left_arm_topic").as_string();
        right_arm_topic_ = this->get_parameter("right_arm_topic").as_string();
        gripper_motor_topic_ = this->get_parameter("gripper_motor_topic").as_string();
        output_topic_ = this->get_parameter("output_topic").as_string();
        arm_control_topic_ = this->get_parameter("arm_control_topic").as_string();
        left_target_topic_ = this->get_parameter("left_target_topic").as_string();
        right_target_topic_ = this->get_parameter("right_target_topic").as_string();
        publish_rate_hz_ = this->get_parameter("publish_rate_hz").as_double();
        stale_timeout_sec_ = this->get_parameter("stale_timeout_sec").as_double();
        startup_grace_sec_ = this->get_parameter("startup_grace_sec").as_double();

        enable_csv_logging_ = this->get_parameter("enable_csv_logging").as_bool();
        csv_log_directory_ = this->get_parameter("csv_log_directory").as_string();
        csv_log_file_prefix_ = this->get_parameter("csv_log_file_prefix").as_string();
        csv_flush_every_n_ = static_cast<int>(
            this->get_parameter("csv_flush_every_n").as_int());

        left_motor_ids_param_ = this->get_parameter("left_motor_ids").as_integer_array();
        right_motor_ids_param_ = this->get_parameter("right_motor_ids").as_integer_array();
        joint_motor_count_ = static_cast<size_t>(std::max<int64_t>(
            1, this->get_parameter("joint_motor_count").as_int()));
        gripper_motor_count_ = static_cast<size_t>(std::max<int64_t>(
            0, this->get_parameter("gripper_motor_count").as_int()));
        total_motor_count_ = joint_motor_count_ + gripper_motor_count_;

        motion_presets_enabled_ =
            this->get_parameter("motion_presets_enabled").as_bool();
        left_extend_positions_ =
            this->get_parameter("left_extend_positions").as_double_array();
        right_extend_positions_ =
            this->get_parameter("right_extend_positions").as_double_array();
        left_retract_positions_ =
            this->get_parameter("left_retract_positions").as_double_array();
        right_retract_positions_ =
            this->get_parameter("right_retract_positions").as_double_array();
        extend_duration_sec_ = this->get_parameter("extend_duration_sec").as_double();
        retract_duration_sec_ = this->get_parameter("retract_duration_sec").as_double();

        if (publish_rate_hz_ <= 0.0) {
            RCLCPP_WARN(get_logger(), "publish_rate_hz <= 0, reset to 50.0 Hz.");
            publish_rate_hz_ = 50.0;
        }

        if (stale_timeout_sec_ <= 0.0) {
            RCLCPP_WARN(get_logger(), "stale_timeout_sec <= 0, reset to 0.5 sec.");
            stale_timeout_sec_ = 0.5;
        }

        if (startup_grace_sec_ < 0.0) {
            RCLCPP_WARN(get_logger(), "startup_grace_sec < 0, reset to 30.0 sec.");
            startup_grace_sec_ = 30.0;
        }

        if (csv_flush_every_n_ <= 0) {
            RCLCPP_WARN(get_logger(), "csv_flush_every_n <= 0, reset to 50.");
            csv_flush_every_n_ = 50;
        }

        if (left_motor_ids_param_.empty()) {
            RCLCPP_WARN(get_logger(), "left_motor_ids is empty.");
        }

        if (right_motor_ids_param_.empty()) {
            RCLCPP_WARN(get_logger(), "right_motor_ids is empty.");
        }

        last_left_rx_time_ = this->now();
        last_right_rx_time_ = this->now();
        start_time_ = this->now();

        left_received_ = false;
        right_received_ = false;

        if (enable_csv_logging_) {
            init_csv_logger();
        }

        auto qos = rclcpp::QoS(10);

        left_sub_ = this->create_subscription<hal::msg::HalArmmotor>(
            left_arm_topic_,
            qos,
            std::bind(
                &ArmmotorAggregatorNode::left_arm_callback,
                this,
                std::placeholders::_1));

        right_sub_ = this->create_subscription<hal::msg::HalArmmotor>(
            right_arm_topic_,
            qos,
            std::bind(
                &ArmmotorAggregatorNode::right_arm_callback,
                this,
                std::placeholders::_1));

        if (gripper_motor_count_ > 0) {
            gripper_sub_ = this->create_subscription<hal::msg::HalArmmotor>(
                gripper_motor_topic_,
                qos,
                std::bind(
                    &ArmmotorAggregatorNode::gripper_callback,
                    this,
                    std::placeholders::_1));
        }

        arm_control_sub_ = this->create_subscription<std_msgs::msg::UInt8>(
            arm_control_topic_,
            qos,
            std::bind(
                &ArmmotorAggregatorNode::arm_control_callback,
                this,
                std::placeholders::_1));

        left_target_pub_ =
            this->create_publisher<trajectory_msgs::msg::JointTrajectoryPoint>(
                left_target_topic_, qos);
        right_target_pub_ =
            this->create_publisher<trajectory_msgs::msg::JointTrajectoryPoint>(
                right_target_topic_, qos);

        merged_pub_ = this->create_publisher<hal::msg::HalArmmotor>(
            output_topic_,
            qos);

        auto period_ms = std::chrono::milliseconds(
            static_cast<int>(1000.0 / std::max(1.0, publish_rate_hz_)));

        timer_ = this->create_wall_timer(
            period_ms,
            std::bind(&ArmmotorAggregatorNode::timer_callback, this));

        RCLCPP_INFO(get_logger(), "ArmmotorAggregatorNode started.");
        RCLCPP_INFO(get_logger(), "Subscribe left : %s", left_arm_topic_.c_str());
        RCLCPP_INFO(get_logger(), "Subscribe right: %s", right_arm_topic_.c_str());
        RCLCPP_INFO(get_logger(), "Publish merged: %s", output_topic_.c_str());
        RCLCPP_INFO(
            get_logger(),
            "Motor aggregation configured: joint=%zu, gripper=%zu, total=%zu.",
            joint_motor_count_, gripper_motor_count_, total_motor_count_);
        RCLCPP_INFO(get_logger(), "Arm control: %s", arm_control_topic_.c_str());
        if (!motion_presets_enabled_) {
            RCLCPP_WARN(
                get_logger(),
                "Motion presets are disabled; commands 0x03/0x04 will be rejected until "
                "verified poses are configured.");
        }
        RCLCPP_INFO(
            get_logger(),
            "Startup grace time: %.2f sec.",
            startup_grace_sec_);

        if (enable_csv_logging_) {
            RCLCPP_INFO(get_logger(), "CSV logging enabled: %s", csv_log_path_.c_str());
        }
    }

    ~ArmmotorAggregatorNode() override
    {
        if (csv_file_.is_open()) {
            csv_file_.flush();
            csv_file_.close();
        }
    }

private:
    void left_arm_callback(const hal::msg::HalArmmotor::SharedPtr msg)
    {
        latest_left_msg_ = *msg;
        left_received_ = true;
        last_left_rx_time_ = this->now();
    }

    void right_arm_callback(const hal::msg::HalArmmotor::SharedPtr msg)
    {
        latest_right_msg_ = *msg;
        right_received_ = true;
        last_right_rx_time_ = this->now();
    }

    void gripper_callback(const hal::msg::HalArmmotor::SharedPtr msg)
    {
        latest_gripper_msg_ = *msg;
        gripper_received_ = true;
        last_gripper_rx_time_ = this->now();
    }

    bool valid_pose(const std::vector<double>& pose) const
    {
        return pose.size() == 5 &&
            std::all_of(pose.begin(), pose.end(), [](double value) {
                return std::isfinite(value);
            });
    }

    void publish_arm_pose(
        const std::vector<double>& left_pose,
        const std::vector<double>& right_pose,
        double duration_sec,
        const char* action_name)
    {
        if (!motion_presets_enabled_) {
            RCLCPP_ERROR(
                get_logger(),
                "Reject arm %s: motion_presets_enabled is false.",
                action_name);
            return;
        }

        if (!valid_pose(left_pose) || !valid_pose(right_pose) ||
            !std::isfinite(duration_sec) || duration_sec <= 0.0)
        {
            RCLCPP_ERROR(
                get_logger(),
                "Reject arm %s: both poses must contain 5 finite values and duration "
                "must be positive.",
                action_name);
            return;
        }

        trajectory_msgs::msg::JointTrajectoryPoint left_msg;
        trajectory_msgs::msg::JointTrajectoryPoint right_msg;
        left_msg.positions = left_pose;
        right_msg.positions = right_pose;
        const int64_t duration_nanoseconds =
            static_cast<int64_t>(std::llround(duration_sec * 1e9));
        const int32_t duration_seconds =
            static_cast<int32_t>(duration_nanoseconds / 1000000000LL);
        const uint32_t remaining_nanoseconds =
            static_cast<uint32_t>(duration_nanoseconds % 1000000000LL);

        left_msg.time_from_start.sec = duration_seconds;
        left_msg.time_from_start.nanosec = remaining_nanoseconds;
        right_msg.time_from_start = left_msg.time_from_start;

        left_target_pub_->publish(left_msg);
        right_target_pub_->publish(right_msg);
        RCLCPP_INFO(
            get_logger(),
            "Arm %s targets published, duration=%.3f sec.",
            action_name,
            duration_sec);
    }

    void arm_control_callback(const std_msgs::msg::UInt8::SharedPtr msg)
    {
        switch (msg->data) {
            case 0x03:
                publish_arm_pose(
                    left_extend_positions_, right_extend_positions_,
                    extend_duration_sec_, "extend");
                break;
            case 0x04:
                publish_arm_pose(
                    left_retract_positions_, right_retract_positions_,
                    retract_duration_sec_, "retract");
                break;
            case 0x05:
                data_upload_enabled_.store(true);
                RCLCPP_INFO(get_logger(), "Armmotor data upload enabled.");
                break;
            case 0x06:
                data_upload_enabled_.store(false);
                RCLCPP_INFO(get_logger(), "Armmotor data upload disabled.");
                break;
            case 0x01:
            case 0x02:
                RCLCPP_INFO(
                    get_logger(),
                    "Cabin command 0x%02X reserved for the cabin motor module.",
                    msg->data);
                break;
            default:
                RCLCPP_WARN(
                    get_logger(), "Unknown arm control command: 0x%02X", msg->data);
                break;
        }
    }

    std::string make_timestamp_string() const
    {
        const auto now = std::chrono::system_clock::now();
        const auto time = std::chrono::system_clock::to_time_t(now);

        std::tm tm_snapshot{};
#if defined(_WIN32)
        localtime_s(&tm_snapshot, &time);
#else
        localtime_r(&time, &tm_snapshot);
#endif

        std::ostringstream oss;
        oss << std::put_time(&tm_snapshot, "%Y%m%d_%H%M%S");
        return oss.str();
    }

    void init_csv_logger()
    {
        try {
            std::filesystem::create_directories(csv_log_directory_);

            std::filesystem::path file_path =
                std::filesystem::path(csv_log_directory_) /
                (csv_log_file_prefix_ + "_" + make_timestamp_string() + ".csv");

            csv_log_path_ = file_path.string();
            csv_file_.open(csv_log_path_, std::ios::out | std::ios::trunc);

            if (!csv_file_.is_open()) {
                RCLCPP_ERROR(
                    get_logger(),
                    "Failed to open CSV log file: %s",
                    csv_log_path_.c_str());
                enable_csv_logging_ = false;
                return;
            }

            csv_file_
                << "timestamp_ns,motor_id,current,speed,position,temp,error,hal_comm_error\n";
            csv_file_.flush();
        } catch (const std::exception& e) {
            RCLCPP_ERROR(get_logger(), "Failed to initialize CSV logger: %s", e.what());
            enable_csv_logging_ = false;
        }
    }

    template<typename T>
    T get_or_default(const std::vector<T>& values, size_t index, T default_value) const
    {
        if (index >= values.size()) {
            return default_value;
        }

        return values[index];
    }

    void write_csv_log(const hal::msg::HalArmmotor& msg)
    {
        if (!enable_csv_logging_ || !csv_file_.is_open()) {
            return;
        }

        for (size_t i = 0; i < msg.motor_current.size(); ++i) {
            csv_file_
                << msg.timestamp << ','
                << (i + 1) << ','
                << get_or_default<int16_t>(msg.motor_current, i, 0) << ','
                << get_or_default<int16_t>(msg.motor_speed, i, 0) << ','
                << get_or_default<int16_t>(msg.motor_position, i, 0) << ','
                << get_or_default<uint16_t>(msg.motor_temp, i, 0) << ','
                << static_cast<int>(get_or_default<uint8_t>(msg.motor_error, i, 0)) << ','
                << static_cast<int>(get_or_default<uint8_t>(msg.hal_comm_error, i, 1))
                << '\n';
        }

        ++csv_write_count_;
        if (csv_write_count_ >= csv_flush_every_n_) {
            csv_file_.flush();
            csv_write_count_ = 0;
        }
    }

    void copy_arm_state_to_merged(
        const hal::msg::HalArmmotor& arm_msg,
        const std::vector<int64_t>& motor_ids,
        hal::msg::HalArmmotor& merged_msg)
    {
        for (size_t i = 0; i < motor_ids.size(); ++i) {
            int64_t motor_id = motor_ids[i];

            if (motor_id < 1 || static_cast<size_t>(motor_id) > joint_motor_count_) {
                RCLCPP_WARN_THROTTLE(
                    get_logger(),
                    *get_clock(),
                    2000,
                    "Invalid joint motor_id=%ld. Valid range is [1, %zu].",
                    static_cast<long>(motor_id), joint_motor_count_);
                continue;
            }

            size_t out_idx = static_cast<size_t>(motor_id - 1);

            if (i < arm_msg.motor_current.size()) {
                merged_msg.motor_current[out_idx] = arm_msg.motor_current[i];
            }

            if (i < arm_msg.motor_speed.size()) {
                merged_msg.motor_speed[out_idx] = arm_msg.motor_speed[i];
            }

            if (i < arm_msg.motor_position.size()) {
                merged_msg.motor_position[out_idx] = arm_msg.motor_position[i];
            }

            if (i < arm_msg.motor_temp.size()) {
                merged_msg.motor_temp[out_idx] = arm_msg.motor_temp[i];
            }

            if (i < arm_msg.motor_error.size()) {
                merged_msg.motor_error[out_idx] = arm_msg.motor_error[i];
            }

            if (i < arm_msg.hal_comm_error.size()) {
                merged_msg.hal_comm_error[out_idx] = arm_msg.hal_comm_error[i];
            }
        }
    }

    void copy_gripper_state_to_merged(
        const hal::msg::HalArmmotor& gripper_msg,
        hal::msg::HalArmmotor& merged_msg)
    {
        for (size_t i = 0; i < gripper_motor_count_; ++i) {
            const size_t out_idx = joint_motor_count_ + i;
            merged_msg.motor_current[out_idx] =
                get_or_default<int16_t>(gripper_msg.motor_current, i, 0);
            merged_msg.motor_speed[out_idx] =
                get_or_default<int16_t>(gripper_msg.motor_speed, i, 0);
            merged_msg.motor_position[out_idx] =
                get_or_default<int16_t>(gripper_msg.motor_position, i, 0);
            merged_msg.motor_temp[out_idx] =
                get_or_default<uint16_t>(gripper_msg.motor_temp, i, 0);
            merged_msg.motor_error[out_idx] =
                get_or_default<uint8_t>(gripper_msg.motor_error, i, 0);
            merged_msg.hal_comm_error[out_idx] =
                get_or_default<uint8_t>(gripper_msg.hal_comm_error, i, 1);
        }
    }

    void timer_callback()
    {
        auto now = this->now();

        bool left_fresh = false;
        bool right_fresh = false;
        bool gripper_fresh = false;

        if (left_received_) {
            double dt_left = (now - last_left_rx_time_).seconds();
            left_fresh = (dt_left <= stale_timeout_sec_);
        }

        if (right_received_) {
            double dt_right = (now - last_right_rx_time_).seconds();
            right_fresh = (dt_right <= stale_timeout_sec_);
        }

        if (gripper_received_) {
            const double dt_gripper = (now - last_gripper_rx_time_).seconds();
            gripper_fresh = (dt_gripper <= stale_timeout_sec_);
        }

        double since_start = (now - start_time_).seconds();
        bool allow_stale_warning = (since_start > startup_grace_sec_);

        if (allow_stale_warning && !left_fresh) {
            RCLCPP_WARN_THROTTLE(
                get_logger(),
                *get_clock(),
                10000,
                "Left armmotor state is stale or not received.");
        }

        if (allow_stale_warning && !right_fresh) {
            RCLCPP_WARN_THROTTLE(
                get_logger(),
                *get_clock(),
                10000,
                "Right armmotor state is stale or not received.");
        }


        if (allow_stale_warning && gripper_motor_count_ > 0 && !gripper_fresh) {
            RCLCPP_WARN_THROTTLE(
                get_logger(), *get_clock(), 10000,
                "Gripper motor state is stale or not received.");
        }

        // 关闭数据管理只停止最终状态上报；各底层驱动仍持续反馈和安全检查。
        if (!data_upload_enabled_.load()) {
            return;
        }

        hal::msg::HalArmmotor merged_msg;

        merged_msg.timestamp = now.nanoseconds();

        merged_msg.motor_current.assign(total_motor_count_, 0);
        merged_msg.motor_speed.assign(total_motor_count_, 0);
        merged_msg.motor_position.assign(total_motor_count_, 0);
        merged_msg.motor_temp.assign(total_motor_count_, 0);
        merged_msg.motor_error.assign(total_motor_count_, 0);
        merged_msg.hal_comm_error.assign(total_motor_count_, 1);

        if (left_fresh) {
            copy_arm_state_to_merged(
                latest_left_msg_,
                left_motor_ids_param_,
                merged_msg);
        }

        if (right_fresh) {
            copy_arm_state_to_merged(
                latest_right_msg_,
                right_motor_ids_param_,
                merged_msg);
        }


        if (gripper_fresh) {
            copy_gripper_state_to_merged(latest_gripper_msg_, merged_msg);
        }

        write_csv_log(merged_msg);
        merged_pub_->publish(merged_msg);
    }

private:
    std::string left_arm_topic_;
    std::string right_arm_topic_;
    std::string gripper_motor_topic_;
    std::string output_topic_;
    std::string arm_control_topic_;
    std::string left_target_topic_;
    std::string right_target_topic_;

    double publish_rate_hz_{50.0};
    double stale_timeout_sec_{0.5};
    double startup_grace_sec_{30.0};

    bool enable_csv_logging_{false};
    std::string csv_log_directory_{"armmotor_logs"};
    std::string csv_log_file_prefix_{"armmotor"};
    std::string csv_log_path_;
    int csv_flush_every_n_{50};
    int csv_write_count_{0};
    std::ofstream csv_file_;

    std::vector<int64_t> left_motor_ids_param_;
    std::vector<int64_t> right_motor_ids_param_;
    size_t joint_motor_count_{10};
    size_t gripper_motor_count_{0};
    size_t total_motor_count_{10};

    bool motion_presets_enabled_{false};
    std::vector<double> left_extend_positions_;
    std::vector<double> right_extend_positions_;
    std::vector<double> left_retract_positions_;
    std::vector<double> right_retract_positions_;
    double extend_duration_sec_{5.0};
    double retract_duration_sec_{8.0};
    std::atomic<bool> data_upload_enabled_{true};

    rclcpp::Subscription<hal::msg::HalArmmotor>::SharedPtr left_sub_;
    rclcpp::Subscription<hal::msg::HalArmmotor>::SharedPtr right_sub_;
    rclcpp::Subscription<hal::msg::HalArmmotor>::SharedPtr gripper_sub_;
    rclcpp::Subscription<std_msgs::msg::UInt8>::SharedPtr arm_control_sub_;
    rclcpp::Publisher<hal::msg::HalArmmotor>::SharedPtr merged_pub_;
    rclcpp::Publisher<trajectory_msgs::msg::JointTrajectoryPoint>::SharedPtr
        left_target_pub_;
    rclcpp::Publisher<trajectory_msgs::msg::JointTrajectoryPoint>::SharedPtr
        right_target_pub_;
    rclcpp::TimerBase::SharedPtr timer_;

    hal::msg::HalArmmotor latest_left_msg_;
    hal::msg::HalArmmotor latest_right_msg_;
    hal::msg::HalArmmotor latest_gripper_msg_;

    bool left_received_{false};
    bool right_received_{false};
    bool gripper_received_{false};

    rclcpp::Time last_left_rx_time_;
    rclcpp::Time last_right_rx_time_;
    rclcpp::Time last_gripper_rx_time_;
    rclcpp::Time start_time_;
};

int main(int argc, char** argv)
{
    rclcpp::init(argc, argv);

    auto node = std::make_shared<ArmmotorAggregatorNode>();

    rclcpp::spin(node);

    rclcpp::shutdown();

    return 0;
}
