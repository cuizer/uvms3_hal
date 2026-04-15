import rclpy
from rclpy.node import Node
import can
from hal.msg import CanMsgOut, CanMsgIn

class Bridge(Node):
    def __init__(self):
        super().__init__('hal_can_bridge')
        try:
            # 锁定 125k 速率
            self.bus = can.interface.Bus(channel='can0', interface='socketcan', bitrate=125000)
            print(">>> [HAL] CAN 桥接器已就绪 (125k) <<<")
        except Exception as e:
            print(f">>> [ERROR] 无法初始化网卡: {e} <<<")

        self.create_subscription(CanMsgOut, '/hal/canout', self.out_callback, 10)
        self.canin_pub = self.create_publisher(CanMsgIn, '/hal/canin', 10)
        # 10ms 物理读取周期
        self.timer = self.create_timer(0.01, self.receive_can_data)

    def out_callback(self, msg):
        can_frame = can.Message(arbitration_id=msg.id, data=bytes(list(msg.data)[:msg.dlc]), is_extended_id=False)
        try:
            self.bus.send(can_frame, timeout=0.01)
        except:
            pass 

    def receive_can_data(self):
        try:
            while True:
                msg = self.bus.recv(0.0)
                if msg is None: break
                ros_msg = CanMsgIn()
                ros_msg.id = msg.arbitration_id
                ros_msg.dlc = msg.dlc
                ros_msg.data = [0]*8
                for i in range(msg.dlc): ros_msg.data[i] = msg.data[i]
                self.canin_pub.publish(ros_msg)
        except:
            pass

def main():
    rclpy.init()
    node = Bridge()
    rclpy.spin(node)
    rclpy.shutdown()

if __name__ == '__main__':
    main()
