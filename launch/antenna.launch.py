import os
from launch import LaunchDescription
from launch.actions import ExecuteProcess, TimerAction
from launch_ros.actions import LifecycleNode

def generate_launch_description():
    
    # ========================================================================
    # 1. 配置 CAN 总线波特率并激活硬件
    # ========================================================================
    # 将 can0 的波特率设定为 125kbps (125000)
    # 使用 echo nvidia | sudo -S 自动静默输入密码
    setup_can_bus = ExecuteProcess(
        cmd=[
            'bash', '-c',
            'echo nvidia | sudo -S bash -c "ip link set can3 down || true && ip link set can3 type can bitrate 125000 && ip link set can3 up"'
        ],
        output='screen'
    )

    # ========================================================================
    # 2. 定义天线生命周期节点
    # ========================================================================
    antenna_node_name = 'hal_antenna_lifecycle_node'
    antenna_node = LifecycleNode(
        package='hal',
        executable='hal_antennacontrol_node',
        name=antenna_node_name,
        namespace='',
        output='screen'
    )

    # ========================================================================
    # 3. 自动触发状态跃迁
    # ========================================================================
    emit_configure = ExecuteProcess(
        cmd=['ros2', 'lifecycle', 'set', f'/{antenna_node_name}', 'configure'],
        output='screen'
    )
    
    emit_activate = ExecuteProcess(
        cmd=['ros2', 'lifecycle', 'set', f'/{antenna_node_name}', 'activate'],
        output='screen'
    )

    # ========================================================================
    # 4. (新增) 自动下发自定义消息进行测试
    # ========================================================================
    # 这里我们演示发送 cmd_type = 3, target_coeff = 127.5 (升降到中间位置)
    # 注意：这里的字典必须采用单引号包裹，且内部不能带额外的双引号
    emit_test_msg = ExecuteProcess(
        cmd=[
            'ros2', 'topic', 'pub', '--once', '/hal/antenna_cmd', 
            'hal/msg/HalAntenna', '{cmd_type: 3, target_coeff: 127.5'
        ],
        output='screen'
    )

    # ========================================================================
    # 5. 编排执行顺序
    # ========================================================================
    return LaunchDescription([
        setup_can_bus,
        
        TimerAction(
            period=1.5,
            actions=[antenna_node] 
        ),
        
        TimerAction(
            period=3.0,
            actions=[emit_configure]
        ),
        
        TimerAction(
            period=4.5,
            actions=[emit_activate]
        ),
        
        # 在激活之后 1.5 秒 (总耗时 6.0 秒)，自动发送测试指令
        TimerAction(
            period=6.0,
            actions=[emit_test_msg]
        )
    ])
