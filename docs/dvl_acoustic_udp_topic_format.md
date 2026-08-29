# DVL 声学控制 Topic 数据格式

## 1. 当前架构

`hal_dvl_node` 不再直接接收 UDP 数据，也不负责发布 `/hal/dvlcontrol`。

当前职责划分：

| 节点 | 职责 |
| --- | --- |
| `bsp_comm_node` 或其他上层通信节点 | 接收上位机 UDP，解析后发布 `/hal/dvlcontrol`。该部分由其他同学实现。 |
| `hal_dvl_node` | 只订阅 `/hal/dvlcontrol`，根据命令控制 DVL 声学开关，并继续通过串口读取 DVL 速度数据后发布 `/hal/dvl`。 |

数据链路：

```text
上位机 UDP
  -> 上层通信节点
  -> /hal/dvlcontrol
  -> hal_dvl_node
  -> DVL 串口 wcs 控制指令
```

## 2. Topic 定义

```text
Topic: /hal/dvlcontrol
Type:  hal/msg/HalDvlControl
```

消息文件：

```text
# builtin_interfaces/Time stamp

uint8 command

uint8 CMD_DISABLE=0
uint8 CMD_ENABLE=1
uint8 CMD_QUERY=2
```

`stamp` 字段暂时保留设计，但在代码和 msg 中先注释，后续需要接收时间戳时可直接恢复。

## 3. 字段含义

| 字段 | 类型 | 含义 |
| --- | --- | --- |
| `stamp` | `builtin_interfaces/Time` | 预留接收/发布时间戳，目前注释。 |
| `command` | `uint8` | DVL 声学控制命令。 |

`command` 取值：

| 值 | 常量 | 含义 | `hal_dvl_node` 行为 |
| --- | --- | --- | --- |
| `0` | `CMD_DISABLE` | 关闭 DVL 声学 | 下发 `wcs,1500,,n,n\n`，成功后将 DVL 数据标记为不可用。 |
| `1` | `CMD_ENABLE` | 开启 DVL 声学 | 下发 `wcs,1500,,y,n\n`，成功后允许解析 DVL 速度数据。 |
| `2` | `CMD_QUERY` | 查询当前状态 | 只打印当前 `acoustic_enabled` 状态，不下发串口控制指令。 |

## 4. 节点处理约定

- `hal_dvl_node` 只消费 `/hal/dvlcontrol`，不关心该 topic 由哪个节点发布。
- 收到非法 `command` 时只打印 warning，不下发串口指令。
- 收到开启或关闭命令时，节点会等待 DVL 串口返回 `wra` ACK 或 `wrn` NACK。
- 若串口未就绪、写入失败、等待 ACK 超时、收到 NACK 或已有命令正在等待回执，节点会打印对应日志。
- DVL 启动默认声学状态仍由参数 `acoustic_enabled_on_start` 控制。

## 5. 调试示例

开启声学：

```bash
ros2 topic pub --once /hal/dvlcontrol hal/msg/HalDvlControl "{command: 1}"
```

关闭声学：

```bash
ros2 topic pub --once /hal/dvlcontrol hal/msg/HalDvlControl "{command: 0}"
```

查询状态：

```bash
ros2 topic pub --once /hal/dvlcontrol hal/msg/HalDvlControl "{command: 2}"
```
