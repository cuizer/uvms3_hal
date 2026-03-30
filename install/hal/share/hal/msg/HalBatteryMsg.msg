# 电池状态消息
# 节点消息命名: hal_battery_msg
# 消息命名: uvms_battery_data

# 电池状态
byte battery_status

# 电池电流 (单位: 0.1A)
int16 battery_current

# 循环次数
uint16 cycle_count

# 剩余电量 (单位: 0.1AH)
uint16 remain_capacity

# 总电量 (单位: 0.1AH)
uint16 total_capacity

# 开关状态: 0=关闭, 1=打开
byte switch_state
