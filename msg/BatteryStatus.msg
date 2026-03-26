# 电池状态反馈消息
# 从单片机回传的实际状态

# 电池类型：0=12V, 1=24V, 2=72V
uint8 battery_type

# 实际开关状态：0=关闭, 1=打开
bool switch_state

# 电池总电压
float32 total_voltage

# 电池总电流
float32 total_current

# 剩余容量
float32 remain_capacity

# 满电容量
float32 full_capacity

# 荷电状态
float32 soc

# 循环次数
uint16 cycle_count

# 最大单体电压
float32 max_cell_vol

# 最大单体电压对应的电池编号
uint8 max_cell_id

# 最小单体电压
float32 min_cell_vol

# 最小单体电压对应的电池编号
uint8 min_cell_id

# 最高温度
int8 max_temp

# 最低温度
int8 min_temp

# 单体电池数量
uint8 cell_count

# 保护状态
uint32 protection_status

# MOS充电状态
bool mos_charge_state

# MOS放电状态
bool mos_discharge_state

# 单体电压数组（最多32个）
uint16[32] cell_voltages

# 温度数组（最多8个）
int8[8] temperatures

# 时间戳
builtin_interfaces/Time stamp