cmake_minimum_required(VERSION 3.8)
project(hal)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# 1. 查找所有必要依赖
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(rclcpp_lifecycle REQUIRED)
find_package(std_msgs REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(std_srvs REQUIRED)
find_package(trajectory_msgs REQUIRED)
find_package(lifecycle_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(builtin_interfaces REQUIRED)


# 2. 头文件目录
include_directories(
  include
)

# 3. 生成自定义接口
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/HalTailservoMsg.msg"
  "msg/HalWingservoMsg.msg"
  "msg/HalMainthrusterMsg.msg"
  "msg/HalAuxithrusterMsg.msg"
  "srv/HalServocontrolSrv.srv"
  "srv/HalThrustercontrolSrv.srv"
  "msg/HalBattery.msg"
  "srv/HalBatteryControlSrv.srv"
  "msg/HalInertialnaviMsg.msg"
  "msg/HalDvlMsg.msg"
  "msg/HalDepthsensorMsg.msg"
  DEPENDENCIES std_msgs builtin_interfaces
)

# 获取生成的 C++ 接口目标
rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")

# 【核心修复1】定义动态生成的头文件路径（绝不能漏！）
set(GEN_INCLUDE_DIR "$<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/rosidl_generator_cpp>")

# 4. 各节点可执行文件

# 电池管理节点
add_executable(hal_battery_node src/hal_battery_node.cpp)
ament_target_dependencies(hal_battery_node
  rclcpp
  rclcpp_lifecycle
  lifecycle_msgs
  std_msgs
)

# 天线控制节点
add_executable(hal_antennacontrol_node src/hal_antennacontrol_node.cpp)
ament_target_dependencies(hal_antennacontrol_node
  rclcpp
  rclcpp_lifecycle
  lifecycle_msgs
  std_msgs
)

# 灯光控制节点
add_executable(hal_lightcontrol_node src/hal_lightcontrol_node.cpp)
ament_target_dependencies(hal_lightcontrol_node
  rclcpp
  rclcpp_lifecycle
  lifecycle_msgs
  std_msgs
)

# 舵机节点
add_executable(hal_servo_node src/hal_servo_node.cpp)
ament_target_dependencies(hal_servo_node
  rclcpp
  rclcpp_lifecycle
  std_msgs
)

# 推进器节点
add_executable(hal_thruster_node src/hal_thruster_node.cpp)
ament_target_dependencies(hal_thruster_node
  rclcpp
  rclcpp_lifecycle
  std_msgs
  std_srvs
)

# 机械臂节点
add_executable(hal_manipulator_node
  src/hal_arm_node.cpp
)
ament_target_dependencies(hal_manipulator_node
  rclcpp
  rclcpp_lifecycle
  std_msgs
  sensor_msgs
  geometry_msgs
  std_srvs
  trajectory_msgs
  lifecycle_msgs
)
#  编译水深传感器节点 (CAN)
add_executable(hal_depthsensor_node src/hal_depthsensor_node.cpp)
ament_target_dependencies(hal_depthsensor_node 
  rclcpp 
  rclcpp_lifecycle
  std_msgs
)



# 编译 DVL 节点 
add_executable(hal_dvl_node src/hal_dvl_node.cpp)
ament_target_dependencies(hal_dvl_node 
  rclcpp 
  rclcpp_lifecycle 
  std_msgs
)


# 编译惯导节点 
add_executable(hal_inertialnavi_node src/hal_inertialnavi_node.cpp)
ament_target_dependencies(hal_inertialnavi_node 
  rclcpp 
  rclcpp_lifecycle 
  std_msgs
)


# 5. 链接自定义消息 typesupport
target_link_libraries(hal_battery_node ${cpp_typesupport_target})
target_link_libraries(hal_antennacontrol_node ${cpp_typesupport_target})
target_link_libraries(hal_lightcontrol_node ${cpp_typesupport_target})
target_link_libraries(hal_servo_node ${cpp_typesupport_target})
target_link_libraries(hal_thruster_node ${cpp_typesupport_target})
target_link_libraries(hal_depthsensor_node  ${cpp_typesupport_target})
target_link_libraries(hal_dvl_node ${cpp_typesupport_target})
target_link_libraries(hal_inertialnavi_node ${cpp_typesupport_target})

# 如果机械臂节点后续也需要使用本包自定义 msg/srv，可保留下面这行
# target_link_libraries(hal_manipulator_node ${cpp_typesupport_target})

# 6. 安装可执行文件
install(TARGETS
  hal_battery_node
  hal_antennacontrol_node
  hal_lightcontrol_node
  hal_servo_node
  hal_thruster_node
  hal_manipulator_node
  hal_depthsensor_node
  hal_dvl_node
  hal_inertialnavi_node
  DESTINATION lib/${PROJECT_NAME}
)

# 7. 安装资源文件
install(DIRECTORY launch DESTINATION share/${PROJECT_NAME}/)
install(DIRECTORY include/ DESTINATION include/)
install(DIRECTORY config DESTINATION share/${PROJECT_NAME}/)

# 8. 导出依赖
ament_export_dependencies(rosidl_default_runtime)
ament_export_dependencies(
  rclcpp
  rclcpp_lifecycle
  lifecycle_msgs
  std_msgs
  sensor_msgs
  geometry_msgs
  std_srvs
  trajectory_msgs
)

ament_package()