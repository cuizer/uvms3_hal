cmake_minimum_required(VERSION 3.8)
project(hal_light)

# 寻找依赖库
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(std_msgs REQUIRED)

# 添加可执行文件
# 确保你的源代码文件名是 hal_lightcontrol_node.cpp
add_executable(hal_lightcontrol_node src/hal_lightcontrol_node.cpp)

# 链接依赖项
ament_target_dependencies(hal_lightcontrol_node 
  rclcpp 
  rclcpp_lifecycle 
  std_msgs
)

# 关键步骤：将生成的可执行文件安装到 lib 目录下
# 这样 ros2 run 才能找到它
install(TARGETS
  hal_lightcontrol_node
  DESTINATION lib/${PROJECT_NAME}
)

# 导出包信息
ament_package()