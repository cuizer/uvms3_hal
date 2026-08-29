import serial
import time
import threading

# 注意：将其修改为 socat 输出的第二个端口名 (例如 /dev/pts/2)
PORT = '/dev/pts/5'
BAUDRATE = 115200

try:
    ser = serial.Serial(PORT, BAUDRATE, timeout=0.1)
    print(f"假 DVL 硬件已启动，连接在 {PORT}")
except Exception as e:
    print(f"打开端口失败: {e}")
    exit()

def send_velocity_data():
    """后台线程：高频模拟 DVL 发送速度数据"""
    while True:
        # 伪造一条 wrz 格式的数据，y代表底面锁定有效
        # 格式：wrz,vx,vy,vz,状态,*
        mock_data = "wrz,0.15,0.25,-0.10,y,*\r\n"
        ser.write(mock_data.encode('utf-8'))
        time.sleep(0.1) # 10Hz 发送频率

# 启动发送线程
threading.Thread(target=send_velocity_data, daemon=True).start()

# 主线程：监听节点下发的指令
while True:
    try:
        # 读取节点发来的指令
        line = ser.readline().decode('utf-8').strip()
        if line:
            print(f"[硬件端接收] 收到 ROS 节点发来的指令: {line}")
            
            # 模拟收到 wcs 模式切换指令
            if line.startswith("wcs"):
                print("[硬件端动作] 模拟处理配置指令，延迟 0.5 秒后回复 ACK (wra)...")
                time.sleep(0.5) 
                
                # 你可以注释掉下面这行，来故意测试 ROS 节点的 2 秒超时 Bug！
                ser.write("wra\r\n".encode('utf-8')) 
                print("[硬件端发送] wra (成功回执)")
    except Exception as e:
        pass