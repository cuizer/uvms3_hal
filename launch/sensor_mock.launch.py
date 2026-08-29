from launch import LaunchDescription
from launch_ros.actions import LifecycleNode
from launch.actions import EmitEvent, RegisterEventHandler
from launch_ros.events.lifecycle import ChangeState
from launch_ros.event_handlers import OnStateTransition
from launch.event_handlers import OnProcessStart
import launch.events
import lifecycle_msgs.msg


def generate_launch_description():
    """启动 sensor_mock_node (模拟深度/INS/DVL) 并自动配置+激活"""

    node = LifecycleNode(
        package='hal',
        executable='sensor_mock_node',
        name='sensor_mock_node',
        namespace='',
        output='screen',
        parameters=[{
            'publish_rate_hz': 50.0,
            'base_depth': 5.0,
            'depth_noise': 0.05,
            'base_yaw': 0.0,
            'base_pitch': 0.0,
            'base_roll': 0.0,
        }],
    )

    register_configure = RegisterEventHandler(
        OnProcessStart(
            target_action=node,
            on_start=[
                EmitEvent(
                    event=ChangeState(
                        lifecycle_node_matcher=launch.events.matches_action(node),
                        transition_id=lifecycle_msgs.msg.Transition.TRANSITION_CONFIGURE,
                    )
                )
            ]
        )
    )

    register_activate = RegisterEventHandler(
        OnStateTransition(
            target_lifecycle_node=node,
            goal_state='inactive',
            entities=[
                EmitEvent(
                    event=ChangeState(
                        lifecycle_node_matcher=launch.events.matches_action(node),
                        transition_id=lifecycle_msgs.msg.Transition.TRANSITION_ACTIVATE,
                    )
                )
            ]
        )
    )

    return LaunchDescription([node, register_configure, register_activate])
