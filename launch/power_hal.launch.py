from launch import LaunchDescription
from launch_ros.actions import LifecycleNode
from ament_index_python.packages import get_package_share_directory
import os


def generate_launch_description():
    pkg_share = get_package_share_directory('hal')

    left_arm_params = os.path.join(pkg_share, 'config', 'left_arm.yaml')
    right_arm_params = os.path.join(pkg_share, 'config', 'right_arm.yaml')

    left_arm_node = LifecycleNode(
        package='hal',
        executable='hal_manipulator_node',
        name='manipulator_driver',
        namespace='left_arm',
        output='screen',
        parameters=[left_arm_params]
    )

    right_arm_node = LifecycleNode(
        package='hal',
        executable='hal_manipulator_node',
        name='manipulator_driver',
        namespace='right_arm',
        output='screen',
        parameters=[right_arm_params]
    )

    return LaunchDescription([
        left_arm_node,
        right_arm_node,
    ])