#include "hal_binocamera_node/hal_binocamera_node.hpp"
//commit
#include <chrono>
#include <cstring>
#include <depthai/depthai.hpp>
#include <exception>
#include <functional>
#include <memory>
#include <vector>

#include <rclcpp/executors/single_threaded_executor.hpp>

namespace hal_binocamera
{

struct HalBinocameraNode::Impl
{
  std::unique_ptr<dai::Device> device_;
  std::shared_ptr<dai::DataOutputQueue> color_queue_;
  std::shared_ptr<dai::DataOutputQueue> depth_queue_;
};

namespace
{

constexpr char kLeftImageTopic[] = "/uvms/perception/image_raw";
constexpr char kDepthImageTopic[] = "/uvms/perception/depth";
constexpr char kCameraStatusTopic[] = "/uvms/perception/camera_status";
constexpr char kColorStreamName[] = "color";
constexpr char kDepthStreamName[] = "depth";
constexpr std::uint8_t kCameraStatusOk = 0x00;
constexpr std::uint8_t kCameraStatusFault = 0x01;
constexpr int kMaxGrabFailCount = 5;

}  // namespace

HalBinocameraNode::HalBinocameraNode()
: rclcpp_lifecycle::LifecycleNode("hal_binocamera_node"),
  pimpl_(std::make_unique<Impl>()),
  grab_fail_count_(0),
  is_camera_open_(false),
  camera_enabled_(true)
{
  declareParameters();
}

HalBinocameraNode::~HalBinocameraNode()
{
  closeCamera();
}

void HalBinocameraNode::declareParameters()
{
  this->declare_parameter("camera_fps", 10.0);
  this->declare_parameter("color_resolution", std::string("THE_800_P"));
  this->declare_parameter("color_width", 320);
  this->declare_parameter("color_height", 320);
  this->declare_parameter("mono_resolution", std::string("THE_800_P"));
  this->declare_parameter("stereo_confidence_threshold", 200);
  this->declare_parameter("stereo_left_right_check", true);
  this->declare_parameter("stereo_extended_disparity", false);
  this->declare_parameter("stereo_subpixel", false);
  this->declare_parameter("device_mx_id", std::string(""));
  this->declare_parameter("usb_speed", std::string("usb2"));
  this->declare_parameter("grab_period_ms", 100);
  this->declare_parameter("frame_id_left", std::string("oak_left_camera_optical_frame"));
  this->declare_parameter("frame_id_depth", std::string("oak_depth_camera_optical_frame"));
}

CallbackReturn HalBinocameraNode::on_configure(const rclcpp_lifecycle::State &)
{
  left_pub_ = this->create_publisher<sensor_msgs::msg::Image>(
    kLeftImageTopic, rclcpp::SensorDataQoS());
  depth_pub_ = this->create_publisher<sensor_msgs::msg::Image>(
    kDepthImageTopic, rclcpp::SensorDataQoS());
  status_pub_ = this->create_publisher<hal::msg::HalbinocameraMsg>(
    kCameraStatusTopic, rclcpp::SystemDefaultsQoS());

  toggle_camera_service_ =
    this->create_service<hal::srv::HalbinocameraSrv>(
    "~/toggle_camera",
    std::bind(
      &HalBinocameraNode::handleToggleCamera,
      this,
      std::placeholders::_1,
      std::placeholders::_2));

  if (!openCamera()) {
    RCLCPP_ERROR(this->get_logger(), "Failed to open OAK-D-SR camera during configure.");
    return CallbackReturn::FAILURE;
  }

  const auto period_ms = this->get_parameter("grab_period_ms").as_int();
  timer_ = this->create_wall_timer(
    std::chrono::milliseconds(period_ms),
    std::bind(&HalBinocameraNode::captureAndPublish, this));
  timer_->cancel();

  RCLCPP_INFO(this->get_logger(), "hal_binocamera_node configured.");
  return CallbackReturn::SUCCESS;
}

CallbackReturn HalBinocameraNode::on_activate(const rclcpp_lifecycle::State &)
{
  left_pub_->on_activate();
  depth_pub_->on_activate();
  status_pub_->on_activate();

  if (timer_) {
    timer_->reset();
  }

  RCLCPP_INFO(this->get_logger(), "hal_binocamera_node activated.");
  return CallbackReturn::SUCCESS;
}

CallbackReturn HalBinocameraNode::on_deactivate(const rclcpp_lifecycle::State &)
{
  if (timer_) {
    timer_->cancel();
  }

  left_pub_->on_deactivate();
  depth_pub_->on_deactivate();
  status_pub_->on_deactivate();

  RCLCPP_INFO(this->get_logger(), "hal_binocamera_node deactivated.");
  return CallbackReturn::SUCCESS;
}

CallbackReturn HalBinocameraNode::on_cleanup(const rclcpp_lifecycle::State &)
{
  if (timer_) {
    timer_->cancel();
    timer_.reset();
  }

  left_pub_.reset();
  depth_pub_.reset();
  status_pub_.reset();
  toggle_camera_service_.reset();
  closeCamera();

  RCLCPP_INFO(this->get_logger(), "hal_binocamera_node cleaned up.");
  return CallbackReturn::SUCCESS;
}

CallbackReturn HalBinocameraNode::on_shutdown(const rclcpp_lifecycle::State &)
{
  if (timer_) {
    timer_->cancel();
  }

  closeCamera();
  return CallbackReturn::SUCCESS;
}

CallbackReturn HalBinocameraNode::on_error(const rclcpp_lifecycle::State &)
{
  if (timer_) {
    timer_->cancel();
  }

  closeCamera();
  return CallbackReturn::SUCCESS;
}

bool HalBinocameraNode::openCamera()
{
  closeCamera();

  try {
    RCLCPP_INFO(this->get_logger(), "DepthAI step 1: start openCamera");

    dai::Pipeline pipeline;

    RCLCPP_INFO(this->get_logger(), "DepthAI step 2a: create left_camera");
    auto left_camera = pipeline.create<dai::node::ColorCamera>();

    RCLCPP_INFO(this->get_logger(), "DepthAI step 2b: create right_camera");
    auto right_camera = pipeline.create<dai::node::ColorCamera>();

    RCLCPP_INFO(this->get_logger(), "DepthAI step 2c: create stereo");
    auto stereo = pipeline.create<dai::node::StereoDepth>();

    RCLCPP_INFO(this->get_logger(), "DepthAI step 2d: create color_xout");
    auto color_xout = pipeline.create<dai::node::XLinkOut>();

    RCLCPP_INFO(this->get_logger(), "DepthAI step 2e: create depth_xout");
    auto depth_xout = pipeline.create<dai::node::XLinkOut>();

    RCLCPP_INFO(this->get_logger(), "DepthAI step 3: set stream names");
    color_xout->setStreamName(kColorStreamName);
    depth_xout->setStreamName(kDepthStreamName);

    const auto color_width = this->get_parameter("color_width").as_int();
    const auto color_height = this->get_parameter("color_height").as_int();
    const auto camera_fps = static_cast<float>(this->get_parameter("camera_fps").as_double());

    RCLCPP_INFO(
      this->get_logger(),
      "DepthAI step 4: read parameters, preview=%dx%d, fps=%.1f",
      color_width,
      color_height,
      camera_fps);

    const auto color_resolution_str = this->get_parameter("color_resolution").as_string();
    dai::ColorCameraProperties::SensorResolution color_resolution;
    if (color_resolution_str == "THE_1080_P") {
      color_resolution = dai::ColorCameraProperties::SensorResolution::THE_1080_P;
    } else if (color_resolution_str == "THE_1200_P") {
      color_resolution = dai::ColorCameraProperties::SensorResolution::THE_1200_P;
    } else if (color_resolution_str == "THE_4_K") {
      color_resolution = dai::ColorCameraProperties::SensorResolution::THE_4_K;
    } else {
      if (color_resolution_str != "THE_800_P") {
        RCLCPP_WARN(
          this->get_logger(),
          "Unsupported color_resolution '%s', defaulting to THE_800_P.",
          color_resolution_str.c_str());
      }
      color_resolution = dai::ColorCameraProperties::SensorResolution::THE_800_P;
    }

    RCLCPP_INFO(this->get_logger(), "DepthAI step 5: configure left camera CAM_B");
    left_camera->setBoardSocket(dai::CameraBoardSocket::CAM_B);
    left_camera->setResolution(color_resolution);
    left_camera->setPreviewSize(color_width, color_height);
    left_camera->setInterleaved(false);
    left_camera->setColorOrder(dai::ColorCameraProperties::ColorOrder::BGR);
    left_camera->setFps(camera_fps);

    RCLCPP_INFO(this->get_logger(), "DepthAI step 6: configure right camera CAM_C");
    right_camera->setBoardSocket(dai::CameraBoardSocket::CAM_C);
    right_camera->setResolution(color_resolution);
    right_camera->setPreviewSize(color_width, color_height);
    right_camera->setInterleaved(false);
    right_camera->setColorOrder(dai::ColorCameraProperties::ColorOrder::BGR);
    right_camera->setFps(camera_fps);

    RCLCPP_INFO(this->get_logger(), "DepthAI step 7: configure stereo depth");
    stereo->setDefaultProfilePreset(dai::node::StereoDepth::PresetMode::HIGH_DENSITY);
    stereo->initialConfig.setConfidenceThreshold(
      this->get_parameter("stereo_confidence_threshold").as_int());
    stereo->setLeftRightCheck(this->get_parameter("stereo_left_right_check").as_bool());
    stereo->setExtendedDisparity(
      this->get_parameter("stereo_extended_disparity").as_bool());
    stereo->setSubpixel(this->get_parameter("stereo_subpixel").as_bool());
    stereo->setDepthAlign(dai::CameraBoardSocket::CAM_B);
    stereo->setOutputSize(color_width, color_height);

    RCLCPP_INFO(this->get_logger(), "DepthAI step 8: link pipeline");
    left_camera->preview.link(color_xout->input);
    left_camera->isp.link(stereo->left);
    right_camera->isp.link(stereo->right);
    stereo->depth.link(depth_xout->input);

    const auto mx_id = this->get_parameter("device_mx_id").as_string();
    const auto usb_speed_str = this->get_parameter("usb_speed").as_string();
    const auto max_usb_speed =
      (usb_speed_str == "usb3") ? dai::UsbSpeed::SUPER : dai::UsbSpeed::HIGH;

    RCLCPP_INFO(
      this->get_logger(),
      "DepthAI step 9: opening device, mx_id='%s', usb_speed=%s",
      mx_id.c_str(),
      usb_speed_str.c_str());

    if (mx_id.empty()) {
      pimpl_->device_ = std::make_unique<dai::Device>(pipeline, max_usb_speed);
    } else {
      pimpl_->device_ = std::make_unique<dai::Device>(
        pipeline,
        dai::DeviceInfo(mx_id),
        max_usb_speed);
    }

    RCLCPP_INFO(this->get_logger(), "DepthAI step 10: device opened, getting output queues");
    pimpl_->color_queue_ = pimpl_->device_->getOutputQueue(kColorStreamName, 2, false);
    pimpl_->depth_queue_ = pimpl_->device_->getOutputQueue(kDepthStreamName, 2, false);

    grab_fail_count_ = 0;
    is_camera_open_ = true;
    RCLCPP_INFO(
      this->get_logger(),
      "OAK-D-SR camera opened successfully. usb_speed=%s, preview=%dx%d, fps=%.1f",
      usb_speed_str.c_str(),
      color_width,
      color_height,
      camera_fps);
    return true;
  } catch (const std::exception & exception) {
    RCLCPP_ERROR(
      this->get_logger(),
      "Failed to open OAK-D-SR camera: %s",
      exception.what());
    closeCamera();
    return false;
  }
}

void HalBinocameraNode::closeCamera()
{
  if (!pimpl_) {
    return;
  }

  pimpl_->depth_queue_.reset();
  pimpl_->color_queue_.reset();
  pimpl_->device_.reset();

  is_camera_open_ = false;
  grab_fail_count_ = 0;
}

void HalBinocameraNode::captureAndPublish()
{
  if (!left_pub_ || !depth_pub_ || !status_pub_) {
    return;
  }

  if (!left_pub_->is_activated() || !depth_pub_->is_activated() || !status_pub_->is_activated()) {
    return;
  }

  if (!camera_enabled_) {
    return;
  }

  if (!is_camera_open_) {
    publishStatus(kCameraStatusFault);
    RCLCPP_WARN_THROTTLE(
      this->get_logger(),
      *this->get_clock(),
      3000,
      "Camera is not open, attempting reconnection.");
    openCamera();
    return;
  }

  try {
    auto color_frame =
      pimpl_->color_queue_ ? pimpl_->color_queue_->tryGet<dai::ImgFrame>() : nullptr;
    auto depth_frame =
      pimpl_->depth_queue_ ? pimpl_->depth_queue_->tryGet<dai::ImgFrame>() : nullptr;

    if (!color_frame && !depth_frame) {
      publishStatus(kCameraStatusFault);
      ++grab_fail_count_;

      RCLCPP_WARN_THROTTLE(
        this->get_logger(),
        *this->get_clock(),
        2000,
        "No frames received from DepthAI output queues.");

      if (grab_fail_count_ >= kMaxGrabFailCount) {
        RCLCPP_ERROR(
          this->get_logger(),
          "Frame retrieval failed %d times in a row, closing camera for reconnect.",
          grab_fail_count_);
        closeCamera();
      }
      return;
    }

    grab_fail_count_ = 0;
    publishStatus(kCameraStatusOk);

    if (color_frame) {
      const auto width = static_cast<int>(color_frame->getWidth());
      const auto height = static_cast<int>(color_frame->getHeight());
      const auto expected_step = width * 3;
      const auto expected_bytes = static_cast<std::size_t>(expected_step) * height;
      const auto & raw = color_frame->getData();

      if (width > 0 && height > 0 && raw.size() >= expected_bytes) {
        std::vector<std::uint8_t> image_data(raw.begin(), raw.end());
        publishColorImage(image_data, width, height, expected_step);
      }
    }

    if (depth_frame) {
      const auto width = static_cast<int>(depth_frame->getWidth());
      const auto height = static_cast<int>(depth_frame->getHeight());
      const auto expected_step = width * static_cast<int>(sizeof(std::uint16_t));
      const auto expected_bytes = static_cast<std::size_t>(expected_step) * height;
      const auto & raw = depth_frame->getData();

      if (width > 0 && height > 0 && raw.size() >= expected_bytes) {
        std::vector<std::uint16_t> depth_data;
        depth_data.reserve(static_cast<std::size_t>(width) * static_cast<std::size_t>(height));
        for (std::size_t i = 0; i + sizeof(std::uint16_t) <= raw.size(); i += sizeof(std::uint16_t)) {
          std::uint16_t pixel_value;
          std::memcpy(&pixel_value, &raw[i], sizeof(std::uint16_t));
          depth_data.push_back(pixel_value);
        }
        publishDepthImage(depth_data, width, height, expected_step);
      }
    }
  } catch (const std::exception & exception) {
    publishStatus(kCameraStatusFault);
    RCLCPP_ERROR(
      this->get_logger(),
      "DepthAI capture error: %s",
      exception.what());
    closeCamera();
  }
}

void HalBinocameraNode::publishStatus(std::uint8_t status_code)
{
  if (!status_pub_ || !status_pub_->is_activated()) {
    return;
  }

  hal::msg::HalbinocameraMsg message;
  message.header.stamp = this->now();
  message.header.frame_id = this->get_parameter("frame_id_left").as_string();
  message.status_code = status_code;
  status_pub_->publish(message);
}

void HalBinocameraNode::handleToggleCamera(
  const std::shared_ptr<hal::srv::HalbinocameraSrv::Request> request,
  std::shared_ptr<hal::srv::HalbinocameraSrv::Response> response)
{
  camera_enabled_ = request->enable;

  if (camera_enabled_) {
    response->success = true;
    response->message = "Camera enabled";
    RCLCPP_INFO(this->get_logger(), "Camera capture and publishing enabled.");
    return;
  }

  response->success = true;
  response->message = "Camera disabled";
  RCLCPP_INFO(this->get_logger(), "Camera capture and publishing disabled.");
}

void HalBinocameraNode::publishColorImage(
  const std::vector<std::uint8_t> & image_data,
  int width,
  int height,
  int stride)
{
  if (image_data.empty() || width <= 0 || height <= 0) {
    return;
  }

  sensor_msgs::msg::Image message;
  message.header.stamp = this->now();
  message.header.frame_id = this->get_parameter("frame_id_left").as_string();
  message.height = height;
  message.width = width;
  message.encoding = "bgr8";
  message.is_bigendian = false;
  message.step = static_cast<sensor_msgs::msg::Image::_step_type>(stride);
  message.data.resize(image_data.size());
  std::memcpy(message.data.data(), image_data.data(), image_data.size());

  left_pub_->publish(message);
}

void HalBinocameraNode::publishDepthImage(
  const std::vector<std::uint16_t> & depth_data,
  int width,
  int height,
  int stride)
{
  if (depth_data.empty() || width <= 0 || height <= 0) {
    return;
  }

  sensor_msgs::msg::Image message;
  message.header.stamp = this->now();
  message.header.frame_id = this->get_parameter("frame_id_depth").as_string();
  message.height = height;
  message.width = width;
  message.encoding = "16UC1";
  message.is_bigendian = false;
  message.step = static_cast<sensor_msgs::msg::Image::_step_type>(stride);
  message.data.resize(depth_data.size() * sizeof(std::uint16_t));
  std::memcpy(message.data.data(), depth_data.data(), depth_data.size() * sizeof(std::uint16_t));

  depth_pub_->publish(message);
}

}  // namespace hal_binocamera

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);

  auto node = std::make_shared<hal_binocamera::HalBinocameraNode>();
  rclcpp::executors::SingleThreadedExecutor executor;
  executor.add_node(node->get_node_base_interface());
  executor.spin();

  rclcpp::shutdown();
  return 0;
}