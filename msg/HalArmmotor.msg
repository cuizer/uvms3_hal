int64 timestamp    #时间戳
int16[] motor_current  #电机电流
int16[] motor_speed    #电机转速
int16[] motor_position   #电电机位置
uint16[] motor_temp    #电机温度 
byte[] motor_error    #错误状态
# HAL 与底层电机通信状态
# 0：HAL 与该电机通信正常
# 1：HAL 与该电机通信丢失
byte[] hal_comm_error