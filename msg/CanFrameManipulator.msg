# //发送请求或接收反馈的时间戳
builtin_interfaces/Time stamp
#AN ID
uint32 can_id
#CAN 数据长度
uint8 dlc
#CAN 数据区，固定 8 字节
uint8[8] data
#优先级
uint8 priority
#来源节点
string source
#帧类型
string frame_type

