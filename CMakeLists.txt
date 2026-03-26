cmake_minimum_required(VERSION 3.8)
project(power_hal)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(lifecycle_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(std_msgs REQUIRED)
find_package(builtin_interfaces REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/BatteryControl.msg"
  "msg/BatteryStatus.msg"
  DEPENDENCIES std_msgs builtin_interfaces
)

add_executable(power_hal_node src/power_hal_node.cpp src/can_interface.cpp)
ament_target_dependencies(power_hal_node
  rclcpp
  rclcpp_lifecycle
  lifecycle_msgs
  std_msgs
)

target_include_directories(power_hal_node PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
)

install(TARGETS
  power_hal_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY
  launch
  DESTINATION share/${PROJECT_NAME}/
)

install(DIRECTORY include/
  DESTINATION include/
)

ament_export_dependencies(rosidl_default_runtime)

rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_link_libraries(power_hal_node ${cpp_typesupport_target})

ament_export_dependencies(
  rclcpp
  rclcpp_lifecycle
  lifecycle_msgs
  std_msgs
)

ament_package()