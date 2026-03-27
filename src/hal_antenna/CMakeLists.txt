cmake_minimum_required(VERSION 3.8)
project(hal_antenna)

if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(std_msgs REQUIRED)

# 生成可执行文件 (确保路径指向你那个 .cpp 文件的实际名字)
add_executable(hal_antennacontrol_node src/hal_antennacontrol_node.cpp)

# 添加依赖
ament_target_dependencies(hal_antennacontrol_node
  rclcpp
  rclcpp_lifecycle
  std_msgs
)

# 安装位置（必须写，否则 ros2 run 找不到）
install(TARGETS
  hal_antennacontrol_node
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()