# 电池控制指令消息
# 来自BSP层的控制指令

# 电池类型：0=12V, 1=24V, 2=72V
uint8 battery_type

# 开关状态：0=关闭, 1=打开
bool switch_state

# 时间戳
builtin_interfaces/Time stamp
