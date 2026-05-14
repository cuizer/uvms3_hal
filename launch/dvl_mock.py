import serial
import time
import math

# 使用 socat 创建的虚拟串口发送端
SERIAL_PORT = '/tmp/ttyDVL_sim' 

def main():
    try:
        ser = serial.Serial(SERIAL_PORT, 115200, timeout=1)
        print(f"DVL 模拟器已启动，正在写入: {SERIAL_PORT}")
        
        t = 0.0
        while True:
            # 构造模拟数据 (正弦波波动)
            vx = 1.2 + 0.1 * math.sin(t)
            vy = 0.05 * math.cos(t)
            vz = 0.01 * math.sin(t)
            valid = 'y' # 模拟底面锁定状态
            
            # 严格对应 C++ 解析逻辑: tokens[0]=wrz, [1]=vx, [2]=vy, [3]=vz, [4]=valid
            payload = f"wrz,{vx:.3f},{vy:.3f},{vz:.3f},{valid}"
            
            # 计算校验和 (虽然 C++ 目前跳过校验，但为了规范建议加上)
            checksum = 0
            for char in payload:
                checksum ^= ord(char)
            
            msg = f"{payload}*{checksum:02X}\r\n"
            
            ser.write(msg.encode('utf-8'))
            time.sleep(0.1) # 10Hz 频率发送
            t += 0.1
            
            if int(t*10) % 10 == 0:
                print(f"发送数据: {msg.strip()}")
                
    except Exception as e:
        print(f"错误: {e}. 请确保执行了 socat 命令！")

if __name__ == '__main__':
    main()