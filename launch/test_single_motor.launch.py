from launch import LaunchDescription
from launch_ros.actions import LifecycleNode, Node
from ament_index_python.packages import get_package_share_directory
import os


def generate_launch_description():
    # 你的包名是 hal！不是 uvms_hal_manipulator！
    pkg_name = 'hal'
    pkg_share = get_package_share_directory(pkg_name)

    # 配置文件路径
    test_config = os.path.join(pkg_share, 'config', 'test_single_motor.yaml')

    # 单电机驱动节点
    single_arm_node = LifecycleNode(
        package='hal',
        executable='manipulator_driver',
        name='manipulator_driver',
        namespace='left_arm',
        output='screen',
        parameters=[test_config]
    )

    # 双臂故障管理节点
    dual_arm_manager_node = Node(
        package='hal',
        executable='dual_arm_lifecycle_manager',
        name='dual_arm_lifecycle_manager',
        output='screen'
    )

    return LaunchDescription([
        single_arm_node,
        dual_arm_manager_node,
    ])