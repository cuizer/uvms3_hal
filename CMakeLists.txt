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
find_package(lifecycle_msgs REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(std_msgs REQUIRED)
find_package(builtin_interfaces REQUIRED)

# 2. 生成自定义接口 
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/HalTailservoMsg.msg"
  "msg/HalWingservoMsg.msg"
  "msg/HalMainthrusterMsg.msg"
  "msg/HalAuxithrusterMsg.msg"
  "srv/HalServocontrolSrv.srv"
  "srv/HalThrustercontrolSrv.srv"
  "msg/HalBatteryMsg.msg"
  "srv/HalBatteryControlSrv.srv"
  DEPENDENCIES std_msgs builtin_interfaces
)

# 获取生成的 C++ 接口目标，用于后续的强绑定
rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")

# 电池管理节点
add_executable(hal_battery_node src/hal_battery_node.cpp)
ament_target_dependencies(hal_battery_node rclcpp rclcpp_lifecycle lifecycle_msgs std_msgs)

# 天线控制节点 (#15)
add_executable(hal_antennacontrol_node src/hal_antennacontrol_node.cpp)
ament_target_dependencies(hal_antennacontrol_node rclcpp rclcpp_lifecycle lifecycle_msgs std_msgs)

# 灯光控制节点 (#14)
add_executable(hal_lightcontrol_node src/hal_lightcontrol_node.cpp)
ament_target_dependencies(hal_lightcontrol_node rclcpp rclcpp_lifecycle lifecycle_msgs std_msgs)

# 舵机节点
add_executable(hal_servo_node src/hal_servo_node.cpp)
ament_target_dependencies(hal_servo_node rclcpp rclcpp_lifecycle std_msgs)

# 编译推进器节点
add_executable(hal_thruster_node src/hal_thruster_node.cpp)
ament_target_dependencies(hal_thruster_node rclcpp rclcpp_lifecycle std_msgs)

# 链接消息类型支持 (确保所有节点都能识别自定义 msg/srv) 
rosidl_get_typesupport_target(cpp_typesupport_target ${PROJECT_NAME} "rosidl_typesupport_cpp")
target_link_libraries(hal_battery_node ${cpp_typesupport_target})
target_link_libraries(hal_antennacontrol_node ${cpp_typesupport_target})
target_link_libraries(hal_lightcontrol_node ${cpp_typesupport_target})
target_link_libraries(hal_servo_node ${cpp_typesupport_target}) 
target_link_libraries(hal_thruster_node ${cpp_typesupport_target}) 


# 5. 安装配置
install(TARGETS
  hal_battery_node
  hal_antennacontrol_node
  hal_lightcontrol_node
  hal_servo_node    
  hal_thruster_node
  DESTINATION lib/${PROJECT_NAME}
)

install(DIRECTORY launch DESTINATION share/${PROJECT_NAME}/)
install(DIRECTORY include/ DESTINATION include/)

ament_export_dependencies(rosidl_default_runtime)
ament_export_dependencies(rclcpp rclcpp_lifecycle lifecycle_msgs std_msgs)

ament_package()
