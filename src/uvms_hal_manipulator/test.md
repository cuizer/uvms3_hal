# UVMS 双臂 HAL 生命周期节点测试文档（ROS 2 + vCAN）

本文档用于在**未连接真实机械臂**的情况下，对 `uvms_hal_manipulator` 软件包进行完整测试，验证以下功能：

* 双臂 HAL 生命周期节点是否能正常启动
* 生命周期是否可以手动切换（configure / activate）
* 关节命令接口是否正常
* 状态反馈接口是否正常
* 急停服务是否正常
* 关节状态与末端位姿是否持续发布
* 底层 CAN 通信链路是否可用（使用 vcan）

---

# 一、环境准备

## 1. 进入工作空间

```bash
cd ~/uvms/uvms_ws
```

## 2. 加载 ROS 2 环境

```bash
source /opt/ros/humble/setup.bash
```

## 3. 加载工作空间环境

```bash
source ~/uvms/uvms_ws/install/setup.bash
```

---

# 二、创建虚拟 CAN 接口（vcan）

由于当前没有真实 CAN 硬件，因此使用 Linux 虚拟 CAN 进行测试。

## 1. 加载 vcan 模块

```bash
sudo modprobe vcan
```

## 2. 创建 vcan0 / vcan1

```bash
sudo ip link add dev vcan0 type vcan
sudo ip link add dev vcan1 type vcan
```

## 3. 启用 vcan

```bash
sudo ip link set up vcan0
sudo ip link set up vcan1
```

## 4. 检查是否创建成功

```bash
ip link show vcan0
ip link show vcan1
```

如果接口存在，说明虚拟 CAN 创建成功。

---

# 三、编译软件包

```bash
cd ~/uvms/uvms_ws
colcon build --packages-select uvms_hal_manipulator
source install/setup.bash
```

---

# 四、启动双臂 HAL 生命周期节点（手动生命周期模式）

```bash
ros2 launch uvms_hal_manipulator dual_arm_hal.launch.py
```

启动后终端应显示：

```text
[left_arm.manipulator_driver]: ManipulatorLifecycleNode created.
[right_arm.manipulator_driver]: ManipulatorLifecycleNode created.
```

---

# 五、检查节点是否启动成功

新开终端执行：

```bash
source ~/uvms/uvms_ws/install/setup.bash
ros2 node list
```

应看到：

```text
/left_arm/manipulator_driver
/right_arm/manipulator_driver
```

---

# 六、检查生命周期初始状态

```bash
ros2 lifecycle get /left_arm/manipulator_driver
ros2 lifecycle get /right_arm/manipulator_driver
```

初始状态应为：

```text
unconfigured [1]
```

---

# 七、手动切换生命周期

## 1. 左臂 configure

```bash
ros2 lifecycle set /left_arm/manipulator_driver configure
```

检查状态：

```bash
ros2 lifecycle get /left_arm/manipulator_driver
```

应为：

```text
inactive [2]
```

## 2. 左臂 activate

```bash
ros2 lifecycle set /left_arm/manipulator_driver activate
```

检查状态：

```bash
ros2 lifecycle get /left_arm/manipulator_driver
```

应为：

```text
active [3]
```

---

## 3. 右臂 configure

```bash
ros2 lifecycle set /right_arm/manipulator_driver configure
```

检查状态：

```bash
ros2 lifecycle get /right_arm/manipulator_driver
```

应为：

```text
inactive [2]
```

## 4. 右臂 activate

```bash
ros2 lifecycle set /right_arm/manipulator_driver activate
```

检查状态：

```bash
ros2 lifecycle get /right_arm/manipulator_driver
```

应为：

```text
active [3]
```

---

# 八、查看系统话题

```bash
ros2 topic list
```

应看到类似话题：

### 左臂

```
/left_arm/hal/manipulator/joint_cmd
/left_arm/hal/manipulator/joint_states
/left_arm/hal/manipulator/end_effector_pose
/left_arm/hal/manipulator/status
```

### 右臂

```
/right_arm/hal/manipulator/joint_cmd
/right_arm/hal/manipulator/joint_states
/right_arm/hal/manipulator/end_effector_pose
/right_arm/hal/manipulator/status
```

---

# 九、发送关节指令测试

## 1. 监听左臂状态

```bash
ros2 topic echo /left_arm/hal/manipulator/status
```

## 2. 发送合法关节指令

```bash
ros2 topic pub /left_arm/hal/manipulator/joint_cmd trajectory_msgs/msg/JointTrajectoryPoint "
positions: [0.1, 0.2, 0.3, 0.5]
velocities: [0.1, 0.1, 0.1, 0.1]
accelerations: []
effort: []
time_from_start: {sec: 0, nanosec: 0}
" --once
```

## 3. 预期结果

状态终端应显示：

```text
[left_arm] Joint command accepted
```

---

# 十、非法命令测试

发送错误关节数量：

```bash
ros2 topic pub /left_arm/hal/manipulator/joint_cmd trajectory_msgs/msg/JointTrajectoryPoint "
positions: [0.1, 0.2, 0.3]
velocities: [0.1, 0.1, 0.1]
accelerations: []
effort: []
time_from_start: {sec: 0, nanosec: 0}
" --once
```

预期状态返回：

```text
Joint command rejected
```

---

# 十一、急停测试

## 1. 触发急停

```bash
ros2 service call /left_arm/hal/manipulator/emergency_stop std_srvs/srv/SetBool "{data: true}"
```

## 2. 急停后发送命令

```bash
ros2 topic pub /left_arm/hal/manipulator/joint_cmd trajectory_msgs/msg/JointTrajectoryPoint "
positions: [0.1, 0.2, 0.3, 0.5]
velocities: [0.1, 0.1, 0.1, 0.1]
accelerations: []
effort: []
time_from_start: {sec: 0, nanosec: 0}
" --once
```

应返回：

```text
Joint command rejected: Emergency stop is active
```

## 3. 解除急停

```bash
ros2 service call /left_arm/hal/manipulator/emergency_stop std_srvs/srv/SetBool "{data: false}"
```

---

# 十二、关节状态发布测试

```bash
ros2 topic echo /left_arm/hal/manipulator/joint_states
```

查看发布频率：

```bash
ros2 topic hz /left_arm/hal/manipulator/joint_states
```

应接近：

```text
50 Hz
```

---

# 十三、末端位姿发布测试

```bash
ros2 topic echo /left_arm/hal/manipulator/end_effector_pose
```

---

# 十四、虚拟 CAN 通信测试

## 1. 监听 CAN

```bash
candump vcan0
```

## 2. 发送 CAN 报文

```bash
cansend vcan0 009#0901011122334455
```

如果 `candump` 能看到报文，说明 CAN 通信正常。

---





