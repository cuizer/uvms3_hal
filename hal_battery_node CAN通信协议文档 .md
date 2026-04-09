# hal\_battery\_node CAN通信协议文档 (v2.0)

## 概述

本文档定义了hal\_battery\_node节点与单片机之间的CAN通信协议，用于控制三个电池（12V、24V、72V）的开关状态，并接收电池的详细BMS状态信息。

## 系统架构

### 双CAN总线设计

- **CAN1 (FDCAN1)**: 主控 ↔ 单片机通信
  - 波特率: **125 kbps**
  - 功能: 控制指令和BMS状态传输
- **CAN2 (FDCAN2)**: 单片机 ↔ BMS通信
  - 波特率: **250 kbps**
  - 功能: 电池管理系统数据采集

### CAN总线配置

| CAN接口  | 波特率      | 用途        |
| ------ | -------- | --------- |
| FDCAN1 | 125 kbps | 主控↔单片机通信  |
| FDCAN2 | 250 kbps | 单片机↔BMS通信 |

- **帧格式**: 标准帧 (11位CAN ID)
- **终端电阻**: CAN总线两端需要120Ω终端电阻

## 主控→单片机协议（FDCAN1）

### 控制指令帧

| 参数         | 值             | 说明                         |
| ---------- | ------------- | -------------------------- |
| CAN ID     | 0x100         | 固定的控制指令ID                  |
| DLC        | 8             | 数据长度                       |
| Data\[0]   | 0x0A          | 命令ID高字节                    |
| Data\[1]   | 0x09          | 命令ID低字节                    |
| Data\[2]   | battery\_type | 电池类型 (0=12V, 1=24V, 2=72V) |
| Data\[3]   | switch\_state | 开关状态 (0x00=OFF, 0x01=ON)   |
| Data\[4-7] | 0x00          | 保留字节                       |

#### 示例数据

**打开12V电池：**
CAN ID: 0x100 DLC: 8 Data: \[0x0A, 0x09, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00]
**关闭24V电池：**
CAN ID: 0x100 DLC: 8 Data: \[0x0A, 0x09, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00]
**打开72V电池：**
CAN ID: 0x100 DLC: 8 Data: \[0x0A, 0x09, 0x02, 0x01, 0x00, 0x00, 0x00, 0x00]

## 单片机→主控协议（FDCAN1）

### BMS状态帧映射

单片机从FDCAN2接收BMS扩展ID数据，映射为标准ID后通过FDCAN1发送给主控。

#### CAN ID映射表

| CAN ID | 数据内容   | 说明                          |
| ------ | ------ | --------------------------- |
| 0x710  | 单体电压组0 | cell\_voltages\[0-3]        |
| 0x711  | 单体电压组1 | cell\_voltages\[4-7]        |
| 0x712  | 单体电压组2 | cell\_voltages\[8-11]       |
| 0x713  | 单体电压组3 | cell\_voltages\[12-15]      |
| 0x714  | 单体电压组4 | cell\_voltages\[16-19]      |
| 0x715  | 单体电压组5 | cell\_voltages\[20-23]      |
| 0x716  | 单体电压组6 | cell\_voltages\[24-27]      |
| 0x717  | 单体电压组7 | cell\_voltages\[28-31]      |
| 0x718  | 单体电压组8 | cell\_voltages\[32-35] (保留) |
| 0x719  | 总体状态   | 总电压、电流、容量                   |
| 0x71A  | SOC状态  | 荷电状态、循环次数                   |
| 0x71B  | 单体电压极值 | 最大/最小单体电压                   |
| 0x71C  | 温度极值   | 最高/最低温度                     |
| 0x71E  | 电池信息   | 单体数量                        |
| 0x723  | 保护状态   | 保护状态、MOS状态                  |
| 0x724  | 温度数组   | temperatures\[0-7]          |
| 0x725  | 其他状态   | 保留                          |

### 详细数据格式

#### 1. 单体电压组 (0x710-0x718)

CAN ID: 0x710 + group\_index DLC: 8 Data\[0-1]: cell\_voltages\[start\_idx] (uint16, 单位mV) Data\[2-3]: cell\_voltages\[start\_idx+1] (uint16, 单位mV) Data\[4-5]: cell\_voltages\[start\_idx+2] (uint16, 单位mV) Data\[6-7]: cell\_voltages\[start\_idx+3] (uint16, 单位mV)
**示例：**
CAN ID: 0x710 Data: \[0x0E, 0x10, 0x0E, 0x20, 0x0E, 0x30, 0x0E, 0x40] 解析：cell\_voltages\[0]=3600mV, cell\_voltages\[1]=3616mV, cell\_voltages\[2]=3632mV, cell\_voltages\[3]=3648mV

#### 2. 总体状态 (0x719)

CAN ID: 0x719 DLC: 8 Data\[0-1]: total\_voltage (uint16, 单位0.1V) Data\[2-3]: total\_current (int16, 单位0.1A) Data\[4-5]: remain\_capacity (uint16, 单位0.1Ah) Data\[6-7]: full\_capacity (uint16, 单位0.1Ah)
**示例：**
CAN ID: 0x719 Data: \[0x2D, 0x00, 0x32, 0x00, 0x64, 0x00, 0xC8, 0x00] 解析：total\_voltage=45.0V, total\_current=5.0A, remain\_capacity=10.0Ah, full\_capacity=20.0Ah

#### 3. SOC状态 (0x71A)

CAN ID: 0x71A DLC: 8 Data\[0-1]: soc (uint16, 单位0.1%) Data\[2-3]: 保留 Data\[4-5]: cycle\_count (uint16) Data\[6-7]: 保留
**示例：**
CAN ID: 0x71A Data: \[0x03, 0x84, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00] 解析：soc=90.0%, cycle\_count=100

#### 4. 单体电压极值 (0x71B)

CAN ID: 0x71B DLC: 8 Data\[0-1]: max\_cell\_vol (uint16, 单位mV) Data\[2]: max\_cell\_id (uint8) Data\[3-4]: min\_cell\_vol (uint16, 单位mV) Data\[5]: min\_cell\_id (uint8) Data\[6-7]: 保留
**示例：**
CAN ID: 0x71B Data: \[0x0E, 0x50, 0x00, 0x0E, 0x00, 0x0F, 0x00, 0x00] 解析：max\_cell\_vol=3664mV, max\_cell\_id=0, min\_cell\_vol=3584mV, min\_cell\_id=15

#### 5. 温度极值 (0x71C)

CAN ID: 0x71C DLC: 8 Data\[0]: max\_temp (int8, 单位℃) Data\[1-3]: 保留 Data\[4]: min\_temp (int8, 单位℃) Data\[5-7]: 保留
**示例：**
CAN ID: 0x71C Data: \[0x23, 0x00, 0x00, 0x00, 0x1E, 0x00, 0x00, 0x00] 解析：max\_temp=35℃, min\_temp=30℃

#### 6. 电池信息 (0x71E)

CAN ID: 0x71E DLC: 8 Data\[0-2]: 保留 Data\[3]: cell\_count (uint8) Data\[4-7]: 保留
**示例：**
CAN ID: 0x71E Data: \[0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x00] 解析：cell\_count=16

#### 7. 保护状态 (0x723)

CAN ID: 0x723 DLC: 8 Data\[0-3]: protection\_status (uint32, 位域) Bit 0: 充电过压保护 Bit 1: 充电过流保护 Bit 2: 放电过流保护 Bit 3: 温度过高保护 Bit 4: 温度过低保护 Bit 5: 单体过压保护 Bit 6: 单体欠压保护 ...其他位保留 Data\[3] Bit 0: mos\_charge\_state (0=关闭, 1=打开) Data\[3] Bit 1: mos\_discharge\_state (0=关闭, 1=打开) Data\[4-7]: 保留
**示例：**
CAN ID: 0x723 Data: \[0x00, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0x00] 解析：protection\_status=0x00000003, mos\_charge\_state=1, mos\_discharge\_state=1

#### 8. 温度数组 (0x724)

CAN ID: 0x724 DLC: 8 Data\[0]: temperatures\[0] (int8, 单位℃) Data\[1]: temperatures\[1] (int8, 单位℃) Data\[2]: temperatures\[2] (int8, 单位℃) Data\[3]: temperatures\[3] (int8, 单位℃) Data\[4]: temperatures\[4] (int8, 单位℃) Data\[5]: temperatures\[5] (int8, 单位℃) Data\[6]: temperatures\[6] (int8, 单位℃) Data\[7]: temperatures\[7] (int8, 单位℃)
**示例：**
CAN ID: 0x724 Data: \[0x1E, 0x1F, 0x20, 0x21, 0x22, 0x23, 0x24, 0x25] 解析：temperatures=\[30, 31, 32, 33, 34, 35, 36, 37]℃

## 单片机固件开发要求

### 1. 接收控制指令（FDCAN1）

单片机需要监听CAN ID 0x100，接收控制指令并执行相应的电池开关操作。

#### 伪代码示例

```c
// FDCAN1接收处理函数
void FDCAN1_Rx_Callback() {
    CAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[8];
    
    if (HAL_FDCAN_GetRxMessage(&hfdcan1, FDCAN_RX_FIFO0, &rx_header, rx_data) == HAL_OK) {
        // 检查是否为控制指令帧
        if (rx_header.Identifier == 0x100) {
            uint8_t cmd_id_high = rx_data[0];
            uint8_t cmd_id_low = rx_data[1];
            
            // 检查命令ID
            if (cmd_id_high == 0x0A && cmd_id_low == 0x09) {
                uint8_t battery_type = rx_data[2];
                uint8_t switch_state = rx_data[3];
                
                // 验证数据有效性
                if (battery_type <= 2) {
                    // 控制对应的电池开关
                    Control_Battery(battery_type, switch_state);
                }
            }
        }
    }
}

// 电池控制函数
void Control_Battery(uint8_t battery_type, bool switch_state) {
    switch(battery_type) {
        case 0: // 12V电池
            HAL_GPIO_WritePin(BATTERY_12V_GPIO_Port, BATTERY_12V_Pin, switch_state);
            break;
        case 1: // 24V电池
            HAL_GPIO_WritePin(BATTERY_24V_GPIO_Port, BATTERY_24V_Pin, switch_state);
            break;
        case 2: // 72V电池
            HAL_GPIO_WritePin(BATTERY_72V_GPIO_Port, BATTERY_72V_Pin, switch_state);
            break;
    }
}
```

### 2. BMS数据转发（FDCAN2 → FDCAN1）

单片机需要从FDCAN2接收BMS数据，映射CAN ID后通过FDCAN1转发给主控。

#### 伪代码示例

```c
// FDCAN2接收处理函数
void FDCAN2_Rx_Callback() {
    CAN_RxHeaderTypeDef rx_header;
    uint8_t rx_data[8];
    
    if (HAL_FDCAN_GetRxMessage(&hfdcan2, FDCAN_RX_FIFO0, &rx_header, rx_data) == HAL_OK) {
        // 映射扩展ID到标准ID
        uint32_t new_std_id = Map_ExtID_To_StdID(rx_header.Identifier);
        
        if (new_std_id != 0x7FF) {
            // 通过FDCAN1转发
            CAN_TxHeaderTypeDef tx_header;
            uint8_t tx_data[8];
            
            tx_header.Identifier = new_std_id;
            tx_header.IdType = FDCAN_STANDARD_ID;
            tx_header.TxFrameType = FDCAN_DATA_FRAME;
            tx_header.DataLength = 8;
            tx_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
            tx_header.BitRateSwitch = FDCAN_BRS_OFF;
            tx_header.FDFormat = FDCAN_CLASSIC_CAN;
            tx_header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
            tx_header.MessageMarker = 0;
            
            memcpy(tx_data, rx_data, 8);
            
            HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan1, &tx_header, tx_data);
        }
    }
}

// 扩展ID到标准ID映射函数
uint32_t Map_ExtID_To_StdID(uint32_t ext_id) {
    if (ext_id >= 0x0E640D09 && ext_id <= 0x0E6C0D09) {
        return 0x710 + ((ext_id - 0x0E640D09) >> 16);
    }
    else if (ext_id >= 0x0A6D0D09 && ext_id <= 0x0A760D09) {
        return 0x719 + ((ext_id - 0x0A6D0D09) >> 16);
    }
    else {
        switch (ext_id) {
            case 0x0AB40D09: return 0x723;
            case 0x0A78090C: return 0x724;
            case 0x0A780C09: return 0x725;
            default: return 0x7FF;
        }
    }
}
```

### 3. 定期查询BMS状态（可选）

单片机可以定期向BMS发送查询命令，获取最新状态。

```c
// 250ms定时任务
void Task_250ms_Report(void) {
    CAN_TxHeaderTypeDef tx_header;
    uint8_t tx_data[8] = {0};
    
    tx_header.Identifier = 0x0E64090D;
    tx_header.IdType = FDCAN_EXTENDED_ID;
    tx_header.TxFrameType = FDCAN_DATA_FRAME;
    tx_header.DataLength = 8;
    tx_header.ErrorStateIndicator = FDCAN_ESI_ACTIVE;
    tx_header.BitRateSwitch = FDCAN_BRS_OFF;
    tx_header.FDFormat = FDCAN_CLASSIC_CAN;
    tx_header.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
    tx_header.MessageMarker = 0;
    
    HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan2, &tx_header, tx_data);
}
```

## ROS2消息和服务定义

### HalBattery消息

```
# 电池状态消息
# 节点消息命名: hal_battery
# 消息命名: uvms_battery_data

byte battery_status      # 电池状态
int16 battery_current    # 电池电流 (0.1A)
uint16 cycle_count       # 循环次数
uint16 remain_capacity   # 剩余电量 (0.1AH)
uint16 total_capacity    # 总电量 (0.1AH)
byte switch_state        # 开关状态: 0=关闭, 1=打开
```

### HalBatteryControlSrv服务

```
# 电池控制服务
# 节点消息命名: hal_batterycontrol
# 消息命名: uvms_batterycontrol_instruction

# 请求部分
# 控制命令：
# 01 = 12V开
# 02 = 12V关
# 03 = 24V开
# 04 = 24V关
# 05 = 72V开
# 06 = 72V关
uint8 command
---
# 响应部分
bool success
string message
```

## 测试方法

### 1. 使用candump监控CAN总线

```bash
# 监控CAN总线流量
candump can0

# 过滤特定CAN ID
candump can0,100:7FF
```

### 2. 使用cansend发送测试帧

```bash
# 发送打开12V电池的指令
cansend can0 100#0A09000100000000

# 发送关闭24V电池的指令
cansend can0 100#0A09010000000000
```

### 3. 使用ROS2节点测试

```bash
# 启动节点（模拟模式）
ros2 run hal hal_battery_node --ros-args -p simulation_mode:=true

# 发送控制指令
ros2 service call /hal/batterycontrol hal/srv/HalBatteryControlSrv "{command: 1}"

# 监听状态反馈
ros2 topic echo /hal/battery
```

## 注意事项

1. **CAN ID冲突**: 确保CAN ID不与其他设备冲突
2. **波特率匹配**:
   - FDCAN1 (主控↔单片机): **125 kbps**
   - FDCAN2 (单片机↔BMS): **250 kbps**
3. **终端电阻**: CAN总线两端需要120Ω终端电阻
4. **数据有效性**: 单片机应验证接收到的数据有效性
5. **错误处理**: 实现完善的CAN错误处理机制
6. **时序要求**: BMS状态转发应尽快完成（建议<100ms）
7. **双CAN隔离**: FDCAN1和FDCAN2应电气隔离，避免干扰

## 附录

### A. 完整CAN ID列表

| CAN ID | 方向     | 数据内容   | 说明                          |
| ------ | ------ | ------ | --------------------------- |
| 0x100  | 主控→单片机 | 控制指令   | 电池开关控制                      |
| 0x710  | 单片机→主控 | 单体电压组0 | cell\_voltages\[0-3]        |
| 0x711  | 单片机→主控 | 单体电压组1 | cell\_voltages\[4-7]        |
| 0x712  | 单片机→主控 | 单体电压组2 | cell\_voltages\[8-11]       |
| 0x713  | 单片机→主控 | 单体电压组3 | cell\_voltages\[12-15]      |
| 0x714  | 单片机→主控 | 单体电压组4 | cell\_voltages\[16-19]      |
| 0x715  | 单片机→主控 | 单体电压组5 | cell\_voltages\[20-23]      |
| 0x716  | 单片机→主控 | 单体电压组6 | cell\_voltages\[24-27]      |
| 0x717  | 单片机→主控 | 单体电压组7 | cell\_voltages\[28-31]      |
| 0x718  | 单片机→主控 | 单体电压组8 | cell\_voltages\[32-35] (保留) |
| 0x719  | 单片机→主控 | 总体状态   | 电压、电流、容量                    |
| 0x71A  | 单片机→主控 | SOC状态  | 荷电状态、循环次数                   |
| 0x71B  | 单片机→主控 | 单体电压极值 | 最大/最小单体电压                   |
| 0x71C  | 单片机→主控 | 温度极值   | 最高/最低温度                     |
| 0x71E  | 单片机→主控 | 电池信息   | 单体数量                        |
| 0x723  | 单片机→主控 | 保护状态   | 保护状态、MOS状态                  |
| 0x724  | 单片机→主控 | 温度数组   | temperatures\[0-7]          |
| 0x725  | 单片机→主控 | 其他状态   | 保留                          |

### B. 电池类型定义

| Battery Type | 电池类型  | 额定电压 |
| ------------ | ----- | ---- |
| 0            | 12V电池 | 12V  |
| 1            | 24V电池 | 24V  |
| 2            | 72V电池 | 72V  |

### C. 开关状态定义

| Switch State | 状态  | 说明   |
| ------------ | --- | ---- |
| 0x00         | OFF | 电池关闭 |
| 0x01         | ON  | 电池打开 |

### D. 保护状态位定义

| 位        | 保护类型   | 说明       |
| -------- | ------ | -------- |
| Bit 0    | 充电过压保护 | 充电电压超过阈值 |
| Bit 1    | 充电过流保护 | 充电电流超过阈值 |
| Bit 2    | 放电过流保护 | 放电电流超过阈值 |
| Bit 3    | 温度过高保护 | 温度超过上限   |
| Bit 4    | 温度过低保护 | 温度低于下限   |
| Bit 5    | 单体过压保护 | 单体电压超过上限 |
| Bit 6    | 单体欠压保护 | 单体电压低于下限 |
| Bit 7-31 | 保留     | 预留扩展     |

### E. 波特率配置

| CAN接口  | 波特率      | 用途        |
| ------ | -------- | --------- |
| FDCAN1 | 125 kbps | 主控↔单片机通信  |
| FDCAN2 | 250 kbps | 单片机↔BMS通信 |

**配置命令示例：**

```bash
# 配置FDCAN1 (125 kbps)
sudo ip link set can0 up type can bitrate 125000

# 配置FDCAN2 (250 kbps) - 如果需要单独配置
sudo ip link set can1 up type can bitrate 250000
```

### F. 操作指南

## 第一步：环境设置

# 1. 进入工作空间

cd /home/epilogue/ros2\_ws

# 2. 设置ROS2环境

source /opt/ros/humble/setup.bash

# 3. 设置工作空间环境

source install/setup.bash

## 第二步：编译

# 1. 清理旧的编译文件（可选）

cd /home/epilogue/ros2\_ws
rm -rf build/ install/ log/

# 2. 编译hal包

colcon build --packages-select hal

# 3. 设置编译后的环境

source install/setup.bash

## 第三步：启动生命周期节点

# 终端1：直接启动节点（模拟模式）

source /home/epilogue/ros2\_ws/install/setup.bash
ros2 run hal hal\_battery\_node --ros-args -p simulation\_mode:=true
ros2 run hal hal_battery_node --ros-args -p simulation_mode:=false

ros2 run hal hal_battery_node --ros-args -p voltage_threshold:=-1.0 -p communication_timeout:=999999.0 -p simulation_mode:=false
## 第四步：配置和激活节点

# 终端2：打开新终端

source /home/epilogue/ros2\_ws/install/setup.bash

source /home/nvidia/test_ws/install/setup.bash

# 1. 查看节点状态

ros2 lifecycle get /hal\_battery\_node

# 2. 配置节点

ros2 lifecycle set /hal\_battery\_node configure

# 3. 激活节点

ros2 lifecycle set /hal\_battery\_node activate

# 4. 查看激活后的状态

ros2 lifecycle get /hal\_battery\_node

## 第五步：测试电池控制功能

# 终端3：打开新终端

source /home/epilogue/ros2\_ws/install/setup.bash

# 打开12V电池

ros2 service call /hal/batterycontrol hal/srv/HalBatteryControlSrv "{command: 1}"

# 等待2秒

sleep 2

# 关闭12V电池

ros2 service call /hal/batterycontrol hal/srv/HalBatteryControlSrv "{command: 2}"

# 等待2秒

sleep 2

# 打开24V电池

ros2 service call /hal/batterycontrol hal/srv/HalBatteryControlSrv "{command: 3}"

# 等待2秒

sleep 2

# 关闭24V电池

ros2 service call /hal/batterycontrol hal/srv/HalBatteryControlSrv "{command: 4}"

# 等待2秒

sleep 2

# 打开72V电池

ros2 service call /hal/batterycontrol hal/srv/HalBatteryControlSrv "{command: 5}"

# 等待2秒

sleep 2

# 关闭72V电池

ros2 service call /hal/batterycontrol hal/srv/HalBatteryControlSrv "{command: 6}"

# 等待2秒

sleep 2

## 第六步：监听电池状态

# 终端4：打开新终端

source /home/epilogue/ros2\_ws/install/setup.bash

# 监听电池状态消息

ros2 topic echo /hal/battery

## 第七步：查看系统信息

# 终端5：打开新终端

source /home/epilogue/ros2\_ws/install/setup.bash

# 1. 查看所有节点

ros2 node list

# 2. 查看所有话题

ros2 topic list

# 3. 查看话题详细信息

ros2 topic info /hal/battery

ros2 topic echo /hal/battery

# 4. 查看服务详细信息

ros2 service info /hal/batterycontrol

# 5. 查看消息类型定义

ros2 interface show hal/msg/HalBattery
ros2 interface show hal/srv/HalBatteryControlSrv

# 6. 查看节点参数

ros2 param list /hal\_battery\_node
ros2 param get /hal\_battery\_node simulation\_mode

### 真实硬件测试

# 1. 配置CAN接口（需要sudo权限）

sudo ip link set can0 up type can bitrate 125000

# 2. 验证CAN接口

ip link show can0

# 3. 启动节点（真实模式）

source install/setup.bash
ros2 run hal hal\_battery\_node --ros-args -p simulation\_mode:=false

# 4. 配置和激活节点

ros2 lifecycle set /hal\_battery\_node configure
ros2 lifecycle set /hal\_battery\_node activate

# 5. 测试控制指令

ros2 service call /hal/batterycontrol hal/srv/HalBatteryControlSrv "{command: 1}"
