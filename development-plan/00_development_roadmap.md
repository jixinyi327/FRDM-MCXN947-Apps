# 开发路线总览

## 1. 目标拆解

FRDM-MCXN947 的核心价值不是“外设多”，而是把双核实时控制、DSP/NPU 加速、工业接口和安全能力放在一颗通用 MCU 中。项目应先明确主矛盾：

| 产品主矛盾 | 优先验证能力 | 关键资源 |
|---|---|---|
| 实时电机/电源控制 | FOC、PWM、ADC、比较器、编码器、故障保护 | FlexPWM、ADC、QDC、SINC、CMP、PowerQuad |
| 工业网关/远程 I/O | Ethernet、CAN FD、Modbus/TCP、MQTT/TLS | ENET、FlexCAN、lwIP、mbedTLS、EdgeLock |
| 边缘视觉 | 摄像头采集、显示、轻量 CNN | SmartDMA、FlexIO、eIQ Neutron N1-16、512 KB SRAM |
| 智能 HMI/家电 | 触摸、显示、语音、低功耗 | TSI、FlexIO/LCD、SAI、PDMIC、低功耗模式 |
| 安全终端 | 安全启动、密钥保护、安全升级 | TrustZone、ELS、PUF、SB3、MCUboot/TF-M |

## 2. 六阶段实施路线

### 阶段 A：平台认知（1 周）

- 阅读数据手册的 Features、Ratings、Power、Pinout、Ordering 部分。
- 用板级手册确认接口、跳线、DNP 和冲突。
- 建立“芯片能力—板卡能力—产品需求”三列矩阵。
- 交付物：需求表、接口预算、初步 BOM 方向。

### 阶段 B：工具链与首跑（1 周）

- 完成 WSL2/Linux 命令行环境。
- 编译 `hello_world`，完成 MCU-Link 烧录和 VCOM 输出。
- 验证调试断点、复位、重新烧录。
- 交付物：可重复构建的脚本、串口截图、版本记录。

### 阶段 C：基础外设与 RTOS（2 至 4 周）

- GPIO 中断、CTimer、PWM、ADC、DMA、UART、SPI、I2C/I3C。
- 引入 FreeRTOS，建立任务、队列、事件、定时器和堆栈监控。
- 用逻辑分析仪/示波器验证时序，而不是只看“程序能跑”。
- 交付物：板级驱动层和最小业务骨架。

### 阶段 D：目标垂直能力（3 至 8 周）

- 工业通信：Ethernet + lwIP + TLS + CAN FD。
- 电机：PWM/ADC/编码器 + FOC + FreeMASTER/MCAT。
- 视觉/语音：SmartDMA 摄像头、显示、NPU 模型部署。
- 安全：TrustZone 分区、安全启动、签名升级。
- 交付物：可演示端到端的垂直原型。

### 阶段 E：自制硬件（4 至 10 周）

- 从参考原理图、BOM、封装和引脚表起步。
- 先做最小系统板和接口风险板，再做完整产品板。
- 按“电源上电、时钟、复位、调试、Flash、通信、模拟”顺序 Bring-up。
- 交付物：原理图评审记录、PCB 叠层、测试点和 Bring-up 报告。

### 阶段 F：产品化（按项目）

- 供应链替代、DNP 清理、量产编程、唯一身份、密钥注入。
- 功能安全/信息安全评估、EMC、环境、寿命、异常注入测试。
- 固件版本、恢复模式、现场升级和回滚策略。
- 交付物：生产测试规范、发布流程、故障恢复流程。

## 3. 项目目录建议

```text
project-root/
├── app/                 业务代码
├── bsp/                 板级初始化、引脚、时钟、外设封装
├── drivers/             自研驱动
├── middleware/          RTOS、网络、文件系统、AI 运行库封装
├── config/              Kconfig、链接脚本、分区表
├── tests/               单元、目标板、压力、异常测试
├── tools/               构建、烧录、模型转换、测试脚本
├── hardware/            原理图、PCB、BOM、叠层、器件库
├── docs/                接口、版本、测试、已知问题
└── sdk/                 固定版本的 SDK，不直接放业务代码
```

业务工程应避免直接修改 SDK 树。若必须修改 SDK，记录补丁和升级迁移步骤。

## 4. 设计门禁

每个阶段结束前至少满足以下条件：

- 所有引脚都有原理图网络名和软件用途。
- 每个共享引脚/焊桥都有明确默认配置和冲突说明。
- 所有电源轨都有电压、电流、上电顺序、去耦和测试点。
- 所有通信接口都有物理层、终端、地回流和故障保护方案。
- 所有 AI/安全/低功耗指标都有量化验收标准。
- 构建、烧录、恢复三条路径均可由第二个人复现。

## 5. 建议的第一批实验

1. `hello_world`：验证工具链、调试器、VCOM。
2. `led_blinky_peripheral` 或 RGB LED：验证 GPIO 和板级映射。
3. I3C 温度传感器：验证 I3C/I2C 和地址 `0x48`。
4. CAN loopback/双节点：先回环，再接收发器。
5. Ethernet DHCP/ping：确认 RMII PHY 和 50 MHz 时钟。
6. FreeRTOS tickless：观察低功耗与唤醒。
7. SmartDMA camera：理解焊桥改动和帧缓冲限制。
8. eIQ TFLM model runner：先跑预置模型，再换自训练模型。
9. PowerQuad FFT：先与软件 FFT 对拍，再测性能。
10. MCUboot/OTA：最后再做，且先准备恢复工具。

## 6. 风险优先顺序

1. 引脚冲突和电源顺序错误，通常会导致“软件看似正常、现场偶发异常”。
2. 摄像头与 Ethernet/CAN 的板级互斥，容易被接口数量误导。
3. 512 KB SRAM 对视觉和 AI 是硬约束；模型、输入、输出、双缓冲必须提前做内存预算。
4. 安全生命周期和熔丝不可逆，必须先在评估板建立受控流程。
5. WSL2 的 USB 直通不是自动完成的；调试链路应从 Windows 侧和 WSL 侧分别验证。

