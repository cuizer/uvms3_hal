import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import LifecycleNode


def generate_launch_description() -> LaunchDescription:
    config_file = os.path.join(
        get_package_share_directory("hal"),
        "config",
        "hal_cabinmotor.yaml",
    )

    cabinmotor_node = LifecycleNode(
        package="hal",
        executable="hal_cabinmotor_node",
        name="hal_cabinmotor_node",
        namespace="",
        output="screen",
        parameters=[config_file],
        emulate_tty=True,
    )

    # 仅创建独立的舱段生命周期节点；当前为0x13单电机测试版本。
    # ros2 lifecycle set /hal_cabinmotor_node configure
    # ros2 lifecycle set /hal_cabinmotor_node activate
    return LaunchDescription([cabinmotor_node])
