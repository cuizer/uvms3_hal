# 下位机遥控控制 Topic 架构与数据格式

## 1. 新架构

统一通信节点 `bsp_comm_node` 负责接收和解析上位机 UDP。运动控制相关节点不再直接接收 UDP，只订阅 ROS topic。

```text
上位机 UDP
  -> bsp_comm_node
  -> /hal/modecontrol
  -> /hal/remotecontrol

/hal/modecontrol
  -> bsp_motioncontrol_node   PID 模式开关
  -> bsp_remotecontrol_node   键盘/开环模式开关

/hal/remotecontrol
  -> bsp_motioncontrol_node   PID 目标映射
  -> bsp_remotecontrol_node   开环推力映射
```

节点职责：

| 节点 | BSP 层名称 | 功能 |
| --- | --- | --- |
| `bsp_comm_node` | 统一通信节点 | 接收 UDP，解析模式命令和遥控通道，发布 ROS topic。 |
| `bsp_motioncontrol_node` | PID 运动控制节点 | 订阅 `/hal/modecontrol` 和 `/hal/remotecontrol`，PID 模式使能后输出 `/hal/thruster/cmd`。 |
| `bsp_remotecontrol_node` | 开环遥控节点 | 订阅 `/hal/modecontrol` 和 `/hal/remotecontrol`，键盘/开环模式使能后输出 `/hal/thruster/cmd`。 |

`bsp_motioncontrol_node` 和 `bsp_remotecontrol_node` 都会发布 `/hal/thruster/cmd`，因此同一时刻只能使能一种模式。

## 2. 模式控制 Topic

```text
Topic: /hal/modecontrol
Type:  hal/msg/HalModeControl
```

`msg/HalModeControl.msg`：

```text
# builtin_interfaces/Time stamp

uint8 command

uint8 CMD_ENABLE_PID=1
uint8 CMD_DISABLE_PID=2
uint8 CMD_ENABLE_KEYBOARD=3
uint8 CMD_DISABLE_KEYBOARD=4
```

字段含义：

| 字段 | 含义 |
| --- | --- |
| `stamp` | 接收时间，当前在 msg 中注释保留，后续需要时可恢复。 |
| `command` | 模式控制命令。 |

`command` 取值：

| 值 | 常量 | 处理节点 | 含义 |
| --- | --- | --- | --- |
| `1` | `CMD_ENABLE_PID` | `bsp_motioncontrol_node` | 开启 PID 闭环运动控制。 |
| `2` | `CMD_DISABLE_PID` | `bsp_motioncontrol_node` | 关闭 PID 闭环运动控制，并输出零推力。 |
| `3` | `CMD_ENABLE_KEYBOARD` | `bsp_remotecontrol_node` | 开启键盘/上位机开环遥控。 |
| `4` | `CMD_DISABLE_KEYBOARD` | `bsp_remotecontrol_node` | 关闭键盘/上位机开环遥控，并输出零推力。 |

## 3. 遥控通道 Topic

```text
Topic: /hal/remotecontrol
Type:  hal/msg/HalRemoteControl
```

`msg/HalRemoteControl.msg`：

```text
# builtin_interfaces/Time stamp

float32 surge
float32 sway
float32 heave
float32 yaw
float32 pitch
float32 roll
```

字段含义：

| 字段 | 含义 |
| --- | --- |
| `stamp` | 接收时间，当前在 msg 中注释保留，后续需要时可恢复。 |
| `surge` | 纵荡通道原始量。 |
| `sway` | 横荡通道原始量。 |
| `heave` | 垂荡/深度通道原始量。 |
| `yaw` | 偏航通道原始量。 |
| `pitch` | 纵倾通道原始量，当前保留。 |
| `roll` | 横滚通道原始量，当前保留。 |

## 4. 通道量范围

所有遥控通道统一使用遥控器原始量：

```text
最小值: 353
中位值: 1024
最大值: 1695
```

归一化公式：

```text
u = clamp((channel - 1024) / 671, -1.0, 1.0)
```

对应关系：

| 原始值 | 归一化值 |
| --- | --- |
| `353` | `-1.0` |
| `1024` | `0.0` |
| `1695` | `1.0` |

当前代码默认死区：

```text
abs(channel - 1024) <= 20 -> 0.0
```

建议 `bsp_comm_node` 或订阅节点对通道量做限幅：

```text
channel < 353  -> 按 353 处理
channel > 1695 -> 按 1695 处理
```

## 5. PID 模式映射

`bsp_motioncontrol_node` 只在收到：

```text
/hal/modecontrol.command = CMD_ENABLE_PID
```

之后消费 `/hal/remotecontrol`。

映射关系：

| 遥控字段 | PID 目标量 | 当前映射 |
| --- | --- | --- |
| `surge` | `vx` | `vx = u_surge * 1.0 m/s` |
| `sway` | `vy` | `vy = u_sway * 0.5 m/s` |
| `heave` | `depth` | `depth = (heave - 353) / 1342 * 10.0 m` |
| `yaw` | `yaw` | `yaw = u_yaw * pi rad` |
| `pitch` | 保留 | 当前强制 `0.0 rad` |
| `roll` | 保留 | 当前强制 `0.0 rad` |

说明：

- `heave=353` 对应 `depth=0.0 m`。
- `heave=1024` 对应 `depth=5.0 m`。
- `heave=1695` 对应 `depth=10.0 m`。
- 当前 PID 实际使用 `vx/vy/depth/yaw`，`pitch/roll` 保留。

## 6. 键盘/开环模式映射

`bsp_remotecontrol_node` 只在收到：

```text
/hal/modecontrol.command = CMD_ENABLE_KEYBOARD
```

之后消费 `/hal/remotecontrol`。

映射关系：

| 遥控字段 | 开环控制量 | 当前映射 |
| --- | --- | --- |
| `surge` | 纵荡力 | `Fx_raw = u_surge * surge_scale` |
| `sway` | 横荡力 | `Fy_raw = u_sway * sway_scale` |
| `heave` | 垂荡力 | `Fz_raw = u_heave * heave_scale` |
| `yaw` | 转艏力矩 | `Mz_raw = u_yaw * yaw_scale` |
| `pitch` | 保留 | 不参与控制 |
| `roll` | 保留 | 不参与控制 |

默认缩放参数：

```text
surge_scale = 300.0
sway_scale  = 160.0
heave_scale = 80.0
yaw_scale   = 5.0
```

`bsp_remotecontrol_node` 会继续执行平滑和推力分配，最后发布：

```text
/hal/thruster/cmd
std_msgs/Float64MultiArray[6]
```

## 7. 安全约定

- `CMD_DISABLE_PID` 和 `CMD_DISABLE_KEYBOARD` 都应触发对应节点输出零推力。
- 遥控通道超时后，对应节点输出零推力。
- 任一通道为 `NaN` 或无穷大时，应拒绝该帧。
- `pitch/roll` 当前保留，上位机建议固定发送中位值 `1024`。
