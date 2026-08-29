#!/usr/bin/env python3
# -*- coding: utf-8 -*-

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, ReliabilityPolicy, HistoryPolicy
import can
import struct
import threading
import time

# 引入自定义的 ROS 2 消息类型
from hal.msg import CanMsgIn, CanMsgOut

class HalCanBridgeNode(Node):
    def __init__(self):
        super().__init__('hal_can_bridge')
        self.get_logger().info(">>> [HAL] 正在初始化 AUV 天线 CAN 桥接器... <<<")

        try:
            # 绑定物理网卡 can3
            self.bus = can.interface.Bus(channel='can3', bustype='socketcan', receive_own_messages=False)
            self.get_logger().info(">>> [HAL] 物理网卡 can3 绑定成功！(波特率: 125k) <<<")
        except Exception as e:
            self.get_logger().error(f"【网络硬伤】无法绑定物理网卡 can3: {str(e)}")
            self.get_logger().error("请检查是否执行了: sudo ip link set can3 up type can bitrate 125000")
            raise e

        qos_profile = QoSProfile(
            reliability=ReliabilityPolicy.RELIABLE,
            history=HistoryPolicy.KEEP_LAST,
            depth=10
        )

        # 订阅 C++ 节点发出的底层数据 (出局链路)
        self.canout_sub = self.create_subscription(
            CanMsgOut,
            '/hal/canout',
            self.canout_callback,
            qos_profile  # <--- 必须带上这个，否则会被静默丢弃！
        )

        # 发布从物理总线接收到的数据给 C++ 节点 (入局链路)
        self.canin_pub = self.create_publisher(CanMsgIn, '/hal/canin', qos_profile)

        self.rx_thread = threading.Thread(target=self.can_rx_loop, daemon=True)
        self.rx_thread.start()

        self.timer = self.create_timer(1.0, self.heartbeat_callback)
        self.get_logger().info(">>> [HAL] CAN 桥接器已就绪，完美的 QoS 通信链路已打通 <<<")

    def heartbeat_callback(self):
        """1Hz 状态自检"""
        self.get_logger().info("【心跳正常】桥接器运行中，时刻监听 C++ 节点的指令...")

    def canout_callback(self, msg: CanMsgOut):
        """
        核心回调：接收 C++ 节点下发的话题，透传给 Linux SocketCAN 驱动
        """
        try:
            # 1. 验证并提取底层数据
            arbitration_id = msg.id
            dlc = msg.dlc
            raw_data = bytes(msg.data[:dlc])

            # 2. 深度拦截日志：翻译 0xA4 / 0xA8 移动指令
            if len(raw_data) > 0 and raw_data[0] in [0xA4, 0xA8]:
                if len(raw_data) >= 8:
                    speed = struct.unpack('<H', raw_data[2:4])[0]
                    target_lsb = struct.unpack('<i', raw_data[4:8])[0]
                    self.get_logger().warn(f"【拦截大数据】指令 0x{raw_data[0]:02X} | 速度: {speed} | 目标位置: {target_lsb} LSB")

            # 3. 封装并强推至 Linux 物理网卡
            can_msg = can.Message(
                arbitration_id=arbitration_id,
                data=raw_data,
                is_extended_id=False
            )
            self.bus.send(can_msg)
            
            # 记录成功发送的基础日志
            hex_str = " ".join(f"{b:02X}" for b in raw_data)
            self.get_logger().info(f"[TX -> can3] ID: 0x{arbitration_id:X} | 数据: [{hex_str}]")

        except can.CanError as e:
            self.get_logger().error(f"【硬件拒发】网卡 SocketCAN 报错: {str(e)}")
        except Exception as e:
            self.get_logger().error(f"【解析异常】Python 内部错误: {str(e)}")

    def can_rx_loop(self):
        """实时读取电机回传的所有数据（RX），并发布给 ROS 2"""
        while rclpy.ok():
            try:
                can_msg = self.bus.recv(0.5)
                if can_msg is None:
                    continue

                ros_msg = CanMsgIn()
                ros_msg.id = can_msg.arbitration_id
                ros_msg.dlc = can_msg.dlc
                ros_msg.data = [0] * 8
                for i in range(min(can_msg.dlc, 8)):
                    ros_msg.data[i] = can_msg.data[i]

                self.canin_pub.publish(ros_msg)
            except Exception as e:
                self.get_logger().error(f"[RX 线程报错] {str(e)}")
                time.sleep(0.1)

def main(args=None):
    rclpy.init(args=args)
    node = HalCanBridgeNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        if hasattr(node, 'bus'):
            node.bus.shutdown()
        node.destroy_node()
        rclpy.shutdown()

if __name__ == '__main__':
    main()
