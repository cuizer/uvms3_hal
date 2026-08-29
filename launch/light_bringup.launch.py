import os
from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import ExecuteProcess, TimerAction, LogInfo

def generate_launch_description():
    print("=========================================")
    print("������ 锁定米文硬件底层物理坐标: gpiochip2 line 79")
    print("=========================================")

    # 1. 核心节点定义 (直接注入已验证的真实底层字符设备坐标)
    light_node = Node(
        package='hal',
        executable='hal_lightcontrol_node',
        name='hal_light_sw_pwm_node',
        output='screen',
        parameters=[{
            'gpio_chip': 'gpiochip0',
            'gpio_offset': 79,
            'active_low': False, # 根据万用表实测，高电平为导通
            'pwm_freq_hz': 50
        }]
    )

    # 2. 自动化操作：延迟 2 秒后，自动触发 Configure 状态
    auto_configure = TimerAction(
        period=2.0,
        actions=[
            LogInfo(msg=">>> 正在自动触发 Configure 状态锁定物理引脚 <<<"),
            ExecuteProcess(
                cmd=['ros2', 'lifecycle', 'set', '/hal_light_sw_pwm_node', 'configure'],
                output='screen'
            )
        ]
    )

    # 3. 自动化操作：延迟 4 秒后，自动触发 Activate 状态
    auto_activate = TimerAction(
        period=4.0,
        actions=[
            LogInfo(msg=">>> 正在自动触发 Activate 状态拉起调光线程 <<<"),
            ExecuteProcess(
                cmd=['ros2', 'lifecycle', 'set', '/hal_light_sw_pwm_node', 'activate'],
                output='screen'
            )
        ]
    )

    return LaunchDescription([
        light_node,
        auto_configure,
        auto_activate
    ])
