#!/usr/bin/env python3
import socket
import struct
import time
import threading
import sys

# ==========================================
# 1. 协议常量与目标配置
# ==========================================
TARGET_IP = "127.0.0.1"
TARGET_PORT = 8114       # Jetson 统一 UDP 入口

MAGIC_UVMC = b'UVMC'     # 协议魔术字
PROTO_VERSION = 1        # 协议版本
CLIENT_ID = 1024         # 模拟的地面站/上位机 ID

# 指令与模式枚举 (严格对齐 C++ 底层)
CMD_SET_MODE = 0x01      # 设置模式指令
CMD_GET_STATUS = 0x02    # 查询状态指令

MODE_STOPPED = 0x00      # 0: 停止/待机模式
MODE_KEYBOARD = 0x01     # 1: 键盘控制模式
MODE_PID = 0x02          # 2: 上位机 PID 目标追踪模式

# 全格状态变量（跨线程共享）
current_mode = MODE_STOPPED
current_cmd = CMD_SET_MODE
request_id_counter = 1

state_lock = threading.Lock()
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

# ==========================================
# 2. 核心算法：CRC-16/MODBUS
# ==========================================
def calculate_crc16(data: bytes) -> int:
    """
    生成 CRC16 校验码 (严格匹配 C++ 端的 CRC-16/MODBUS 实现)
    """
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 1:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc

# ==========================================
# 3. 后台心跳保活线程
# ==========================================
def heartbeat_worker():
    """
    后台线程：以 1Hz 频率持续发送当前模式指令，防止 C++ 端看门狗超时回滚
    """
    global request_id_counter, current_cmd, current_mode
    
    while True:
        with state_lock:
            cmd = current_cmd
            mode = current_mode
            req_id = request_id_counter
            request_id_counter += 1

        # 核心业务安全规则：查询状态或停止模式租期必须为 0，PID 或 键盘模式给 2000ms
        if cmd == CMD_GET_STATUS or mode == MODE_STOPPED:
            lease_ms = 0
        else:
            lease_ms = 2000

        # 打包前 18 字节
        payload_no_crc = struct.pack(
            '<4sBBBBIIH',
            MAGIC_UVMC, PROTO_VERSION, cmd, mode, 0,
            CLIENT_ID, req_id, lease_ms
        )
        
        # 附加上 2 字节 CRC16/MODBUS 校验，凑齐 20 字节
        crc = calculate_crc16(payload_no_crc)
        packet = payload_no_crc + struct.pack('<H', crc)
        
        try:
            sock.sendto(packet, (TARGET_IP, TARGET_PORT))
        except Exception as e:
            print(f"\n[ERROR] 发送数据失败: {e}", file=sys.stderr)
            
        time.sleep(1.0)  # 1Hz 频率保活

# ==========================================
# 4. 主程序：控制台交互
# ==========================================
def main():
    global current_mode
    
    print("=====================================================")
    print("      水下机器人上位机模式切换控制器 (多线程版)       ")
    print("=====================================================")
    print(f"�� 目标节点: {TARGET_IP}:{TARGET_PORT} | 客户端ID: {CLIENT_ID}")
    print("-----------------------------------------------------")
    print("模式代码指南:")
    print("  [0] -> STOPPED  (停机/安全就绪状态)")
    print("  [1] -> KEYBOARD (键盘直接控制模式)")
    print("  [2] -> PID_TGT  (上位机 PID 目标追踪模式)")
    print("=====================================================")

    # 启动后台保活心跳
    bg_thread = threading.Thread(target=heartbeat_worker, daemon=True)
    bg_thread.start()
    print("[INFO] 后台 1Hz 续租心跳线程已启动。\n")

    mode_map = {
        0: ("STOPPED", MODE_STOPPED),
        1: ("KEYBOARD", MODE_KEYBOARD),
        2: ("PID_TGT", MODE_PID)
    }

    try:
        while True:
            user_input = input("请输入目标模式代码 [0, 1, 2] 并回车: ").strip()
            
            if not user_input:
                continue
                
            if not user_input.isdigit() or int(user_input) not in mode_map:
                print("❌ 输入错误！只能输入 0, 1 或 2。\n")
                continue
                
            choice = int(user_input)
            mode_name, mode_val = mode_map[choice]
            
            # 安全线程锁，切换当前全局发送状态
            with state_lock:
                current_mode = mode_val
                
            print(f"▶️>>> 正在请求切换至: {mode_name} 模式，后台将持续以此模式续租... <<<\n")

    except KeyboardInterrupt:
        print("\n\n�� 检测到 Ctrl+C，正在紧急安全停机...")
        with state_lock:
            current_mode = MODE_STOPPED
        
        # 额外强制发送一包停机包确保立马生效
        payload_stop = struct.pack('<4sBBBBIIH', MAGIC_UVMC, PROTO_VERSION, CMD_SET_MODE, MODE_STOPPED, 0, CLIENT_ID, 9999, 0)
        crc = calculate_crc16(payload_stop)
        sock.sendto(payload_stop + struct.pack('<H', crc), (TARGET_IP, TARGET_PORT))
        
        print("✅ 成功发送 STOPPED 停机指令。程序退出。")
    finally:
        sock.close()

if __name__ == "__main__":
    main()
