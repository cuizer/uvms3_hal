from launch import LaunchDescription
from launch_ros.actions import LifecycleNode
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration

def generate_launch_description():
    simulation_mode_arg = DeclareLaunchArgument(
        'simulation_mode',
        default_value='true',
        description='Enable simulation mode (no CAN hardware required)'
    )
    
    return LaunchDescription([
        simulation_mode_arg,
        LifecycleNode(
            package='power_hal',
            executable='hal_battery_node',
            name='hal_battery_node',
            namespace='',
            output='screen',
            parameters=[{
                'simulation_mode': LaunchConfiguration('simulation_mode'),
            }],
        ),
    ])