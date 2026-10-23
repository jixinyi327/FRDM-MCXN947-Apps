# 学习路线与实验清单

## 1. 推荐 12 周路线

### 第 1 周：资料与接口预算

- 数据手册 Features/Power/Pinout；
- 板级手册和原理图；
- 接口并发矩阵；
- 选择一条垂直方向。

交付：一页系统框图、一页接口预算、一页风险表。

### 第 2 周：工具链和首跑

- 环境脚本；
- hello_world；
- LinkServer；
- VCOM；
- 断点/reset。

交付：可重复构建和烧录记录。

### 第 3 周：GPIO/时钟/定时器/低功耗

- RGB、按键、CTimer、LPTimer；
- Sleep/Power Down；
- 电流测量。

交付：低功耗电流表和各模式恢复测试。

### 第 4 周：串行接口

- UART、SPI、I2C、I3C；
- DMA；
- 总线恢复；
- 传感器。

交付：四类接口自测报告。

### 第 5 周：RTOS

- FreeRTOS 任务、队列、信号量、定时器；
- 堆栈水位；
- 优先级反转；
- tickless。

交付：多任务原型和调度分析。

### 第 6 周：网络与 CAN

- PHY、ping、DHCP；
- CAN FD loopback/双节点；
- 网络压力。

交付：CAN-Ethernet 数据转发 Demo。

### 第 7 周：USB/存储

- USB CDC/MSC；
- QSPI；
- SD/FatFs/LittleFS；
- 掉电一致性。

交付：文件记录和断电恢复。

### 第 8 周：显示/触摸/摄像头

- FlexIO LCD；
- TSI；
- 摄像头改焊与单帧；
- SmartDMA。

交付：摄像头 + LCD Demo。

### 第 9 周：AI/DSP

- 预置 TFLM；
- 自训练量化模型；
- PowerQuad FFT；
- 性能/RAM 基线。

交付：AI 推理报告。

### 第 10 周：电机或垂直业务

- 电机：FOC、MCAT、保护；
- 网关：MQTT/TLS/OTA；
- HMI：LVGL。

交付：可演示垂直 PoC。

### 第 11 周：双核与安全

- RPMsg/SEMA42；
- TrustZone；
- 安全启动；
- 签名升级。

交付：双核 + 安全升级方案。

### 第 12 周：自制板评审

- 电源/时钟/接口/BOM；
- PCB 叠层；
- Bring-up 计划；
- 测试夹具方案。

交付：可评审的原理图和 PCB 预审包。

## 2. 基础实验清单

| 编号 | 实验 | 主要资源 | 验收 |
|---|---|---|---|
| B01 | 三色 LED | GPIO | 无毛刺、复位安全 |
| B02 | 按键消抖 | PINT | 无误触发 |
| B03 | CTimer | 定时器 | 抖动测量 |
| B04 | PWM | FlexPWM | 死区/频率 |
| B05 | ADC | ADC/DMA | 线性/串扰 |
| B06 | DAC/CMP | DAC/CMP | 阈值和响应 |
| B07 | UART | FlexComm | 1 MB 无误码 |
| B08 | SPI | LPSPI | 回环/真实设备 |
| B09 | I2C | LPI2C | 总线恢复 |
| B10 | I3C 温度 | I3C1 | 读取和告警 |
| B11 | QSPI Flash | FlexSPI | 全空间校验 |
| B12 | SD/FatFs | uSDHC | 插拔/断电 |
| B13 | USB CDC | USB HS | 枚举/传输 |
| B14 | CAN FD | FlexCAN | 双节点/FD 长帧 |
| B15 | Ethernet | ENET/lwIP | ping/DHCP/TCP |
| B16 | FreeRTOS | RTOS | 堆栈/优先级 |
| B17 | 低功耗 | SPC/WUU | 电流/唤醒 |
| B18 | 看门狗 | WWDT | 复位源 |

## 3. 进阶实验清单

| 编号 | 实验 | 目标 |
|---|---|---|
| A01 | TLS/MQTT | 证书与安全连接 |
| A02 | OTA | 签名、回滚、断电恢复 |
| A03 | SmartDMA 摄像头 | 帧同步和内存 |
| A04 | LVGL | 显示性能 |
| A05 | TSI | 湿手和噪声 |
| A06 | PDM/SAI | 音频链路 |
| A07 | TFLM | 量化模型推理 |
| A08 | PowerQuad | FFT/DSP 性能 |
| A09 | PMSM FOC | 电流/速度/位置 |
| A10 | TrustZone | 安全/非安全隔离 |
| A11 | RPMsg | 双核通信 |
| A12 | SEMA42 | 共享资源保护 |

## 4. 产品化清单

- [ ] 接口并发矩阵冻结。
- [ ] 电源树和上电顺序冻结。
- [ ] 时钟和复位冻结。
- [ ] 硬件原理图评审。
- [ ] PCB 叠层/阻抗评审。
- [ ] 首件 Bring-up。
- [ ] 通信压力和环境测试。
- [ ] 安全生命周期与密钥流程。
- [ ] 量产测试夹具。
- [ ] 固件恢复和现场升级演练。

## 5. 不建议的跳过项

- 直接上摄像头而不检查焊桥；
- 直接接电机而不先测 PWM/ADC/故障关断；
- 先启用不可逆安全配置；
- 用开发板 USB 口判断目标 USB；
- 只看编译成功不烧录；
- 不做断电、插拔和错误恢复；
- 不记录工具版本和 SDK revision；
- 把典型功耗当系统功耗；
- 只看 NPU 峰值不看端到端延迟；
- 第一版自制板同时塞入所有扩展接口。

