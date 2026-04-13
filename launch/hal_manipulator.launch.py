from launch import LaunchDescription
from launch_ros.actions import LifecycleNode, Node
from ament_index_python.packages import get_package_share_directory
import os


def generate_launch_description():
    pkg_share = get_package_share_directory('hal')

    left_arm_params = os.path.join(pkg_share, 'config', 'left_arm.yaml')
    right_arm_params = os.path.join(pkg_share, 'config', 'right_arm.yaml')

    # 左臂生命周期节点
    left_arm_node = LifecycleNode(
        package='hal',
        executable='hal_manipulator_node',
        name='manipulator_driver',
        namespace='left_arm',
        output='screen',
        parameters=[left_arm_params]
    )

    # 右臂生命周期节点
    right_arm_node = LifecycleNode(
        package='hal',
        executable='hal_manipulator_node',
        name='manipulator_driver',
        namespace='right_arm',
        output='screen',
        parameters=[right_arm_params]
    )

    # 双臂生命周期管理节点
    # 作用：
    # 1. 监听左右臂 fault 话题
    # 2. 任意一臂故障时，统一将左右臂切换到 inactive
    # 3. 报警并等待人工介入
    # 4. 不自动 cleanup
    # 5. 不自动重新 configure
    dual_arm_manager_node = Node(
        package='hal',
        executable='dual_arm_lifecycle_manager',
        name='dual_arm_lifecycle_manager',
        output='screen'
    )

    return LaunchDescription([
        left_arm_node,
        right_arm_node,
        dual_arm_manager_node,
    ])