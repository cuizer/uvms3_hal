cmake_minimum_required(VERSION 3.8)
project(uvms_hal_manipulator)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# =========================
# Find dependencies
# =========================
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(std_srvs REQUIRED)
find_package(trajectory_msgs REQUIRED)
find_package(lifecycle_msgs REQUIRED)

# =========================
# Include directories
# =========================
include_directories(
  include
)

# =========================
# Executable
# =========================
add_executable(manipulator_driver
  src/main.cpp
  src/manipulator_lifecycle_node.cpp
  src/protocol_parser.cpp
  src/can_driver.cpp
  src/safety_manager.cpp
)

ament_target_dependencies(manipulator_driver
  rclcpp
  rclcpp_lifecycle
  std_msgs
  sensor_msgs
  geometry_msgs
  std_srvs
  trajectory_msgs
  lifecycle_msgs
)

target_include_directories(manipulator_driver PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

target_compile_features(manipulator_driver PUBLIC c_std_99 cxx_std_17)

# =========================
# Install executable
# =========================
install(TARGETS
  manipulator_driver
  DESTINATION lib/${PROJECT_NAME}
)

# =========================
# Install header files
# =========================
install(DIRECTORY
  include/
  DESTINATION include/
)

# =========================
# Install launch and config
# =========================
install(DIRECTORY
  launch
  config
  DESTINATION share/${PROJECT_NAME}
)

# =========================
# Testing
# =========================
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

ament_package()