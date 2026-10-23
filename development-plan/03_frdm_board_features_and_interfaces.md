# FRDM-MCXN947 开发板特性与接口

## 1. 板卡定位

FRDM-MCXN947 是低成本评估/原型板，尺寸约 118 mm × 55 mm。它把 MCXN947、外部 QSPI Flash、Ethernet PHY、CAN 收发器、I3C 温度传感器、USB Type-C、用户 LED/按键、Arduino/mikroBUS/FlexIO/摄像头接口和 MCU-Link 调试器集成在一块板上。

它的主要用途是：

- 评估芯片性能和外设；
- 跑 SDK 示例；
- 接 FRDM/mikroBUS/Pmod/Arduino 扩展；
- 作为自制硬件的原理图和 BOM 参考；
- 做电机、Ethernet、CAN、AI、HMI 等垂直演示。

## 2. 核心硬件清单

| 类别 | 器件/资源 | 说明 |
|---|---|---|
| MCU | MCXN947VDFT | 双 Cortex-M33 150 MHz，184 VFBGA |
| 5 V 输入 | J17 MCU-Link USB-C、J11 HS USB-C、Arduino 5 V、J15 备用稳压输入 | 默认 J17 供电；避免多个 5 V 源互相顶灌 |
| 3.3 V | XC6227C331PR-G LDO | 默认板级 3.3 V |
| MCU 电源 | DCDC Core + LDO_SYS 等 | 板上默认 DCDC_CORE，提供电流测量/配置器件位置 |
| 系统时钟 | 24 MHz 晶振 | MCU 高精度参考 |
| RTC 时钟 | 32.768 kHz 晶振 | 低功耗时基 |
| Ethernet 时钟 | 50 MHz 有源晶振 | 供 LAN8741 RMII |
| MCU-Link 时钟 | 16 MHz | LPC55S69 调试器 |
| 外部 Flash | W25Q64JVSSIQ，8 MB QSPI | 已装配；U7 Octal Flash 64 MB 为 DNP |
| Ethernet PHY | LAN8741A-EN | RMII，10/100 Mbps |
| CAN PHY | TJA1057GTK/3Z | CAN0，CAN FD 物理层 |
| I3C 传感器 | P3T1755DP | I3C1，7-bit I2C 地址 `0x48` |
| Debug | MCU-Link OB，LPC55S69 | CMSIS-DAP，VCOM，USBSIO；可换 J-Link LITE 固件 |
| HMI | RGB LED、复位/唤醒/ISP 按键、触摸滑条 | 适合最小验证 |

## 3. 电源入口与跳线

| 标识 | 功能 | 默认与风险 |
|---|---|---|
| J17 | MCU-Link USB-C | 默认供电和调试入口；接 PC 即可开始 |
| J11 | MCU 高速 USB-C | 目标 MCU 的 USB HS；也可给板供电 |
| J15 | 备用 5 V 稳压输入 | 默认不装配 |
| J24 | `P3V3_MCU` 由板级 `P3V3` 供电 | 默认 1-2 短接 |
| J18 | MCU-Link VCOM 使能 | 默认开路；短接会禁用 VCOM |
| J19 | 板载 MCU-Link SWD 使能 | 默认开路；使用外部调试器时短接 |
| J21 | MCU-Link 强制 ISP | 默认开路；固件修复时短接并重新上电 |
| J22 | MCU-Link SWD 时钟使能 | 默认短接 |

不要在多个电源入口同时注入互相竞争的 5 V。做电流测量或外部供电前，先依据板级手册电源图确认需要断开哪些 0 Ω/跳线，避免反向供电。

## 4. 连接器地图

| 标识 | 类型 | 用途 | 注意 |
|---|---|---|---|
| J1/J2/J3/J4 | Arduino/FRDM 双排 | 扩展传感器、电机驱动、USB、触摸 | 外排 Arduino，内排 FRDM；存在较多复用 |
| J5/J6 | mikroBUS | Click 扩展 | J5 的 RX/TX 与 I3C 和摄像头共享 |
| J7 | Pmod | SPI/I2C/GPIO 扩展 | 默认 DNP，需自行装配 |
| J8 | FlexIO/LCD 28 pin | LCD-PAR-S035 等 | 必须使用 3.3 V I/O 模块 |
| J9 | 摄像头 32 pin | OV7670 等 | 默认需要改焊才能正确读摄像头 |
| J10 | CAN 4 pin | CAN_H、CAN_L、5 V、GND | 外部终端/总线必须正确 |
| J11 | HS USB Type-C | USB HS Host/Device | 与 J17 不要混淆 |
| J12 | MicroSD | uSDHC | 默认 DNP |
| J16 | RJ45 | Ethernet | RMII 100 Mbps |
| J17 | MCU-Link USB-C | 调试、供电、VCOM | 首次上电首选 |
| J23 | 10 pin SWD/JTAG | 外部调试器 | 需 J19 短接以禁用板载 SWD |

## 5. 板载 I/O 映射

| 功能 | MCU 引脚 | 说明 |
|---|---|---|
| 用户红灯 | P0_10 | 同时复用 Arduino D9 相关网络 |
| 用户绿灯 | P0_27 | 同时复用 Arduino D10/SPI PCS |
| 用户蓝灯 | P1_2 | 同时复用 Arduino D6 与 MCU-Link SPI 桥 |
| 复位键 SW1 | RESET_B | 板上复位 |
| 唤醒键 SW2 | P0_23 / WAKEUP_B | 低电平按下 |
| ISP 键 SW3 | P0_6 / ISPMODE_N-DEBUG | 低电平按下；用于启动模式/恢复 |
| 触摸滑条 | P1_3 | 与 mikroBUS RST、MCU-Link SPI PCS 有共享 |

## 6. 通信外设的实际连接

### UART/VCOM

MCU-Link 的 VCOM 连接到目标 MCU 的 `P1_8/P1_9`。把 J17 接 PC、保持 J18 开路，会看到 MCU-Link VCOM 端口。`P1_8/P1_9` 也是 UART ISP 路径。

### I3C 温度传感器

- 控制器：I3C1；
- 数据/时钟：`P1_16/P1_17`；
- 7-bit I2C 地址：`0x48`；
- 同一网络还连接 mikroBUS J5 的 RX/TX 和摄像头相关引脚。不能默认认为“I3C 传感器 + mikroBUS UART + 摄像头”可同时工作。

### Ethernet

- PHY：LAN8741A-EN；
- 接口：RMII；
- PHY 时钟：板载 50 MHz 晶振；
- MCU 相关信号集中在 P1 端口；
- 摄像头模式会占用与 Ethernet 冲突的 P1 引脚，默认需要改焊。

### CAN

- 控制器：CAN0；
- TX/RX：`P1_10/P1_11`；
- 收发器：TJA1057；
- J10 提供 CAN_H/CAN_L/5 V/GND；
- 摄像头模式与 CAN 的 P1_10/P1_11 也应做焊桥冲突检查。

### USB

目标 MCU 有 HS 和 FS 控制器，但 FRDM 板只把 HS 引到 J11。J17 是 LPC55S69 MCU-Link 的 USB，不是目标 MCU USB。做 USB 实验时一定区分这两个 Type-C。

### 外部 Flash

默认 W25Q64JVSSIQ 通过 FlexSPI 连接，用于 QSPI XIP 和外部存储示例。U7/U8 封装重叠，Octal Flash 默认 DNP。若替换为不同 Flash，需要核对命令集、时序、复位、供电和 XIP 配置。

### MicroSD

J12 默认 DNP。SDHC 信号与 Arduino PWM 及摄像头相关引脚存在复用，正式使用前应设置 Port 2 电压和选择网络。

## 7. 板级互斥与 DNP

| 场景 | 默认限制 | 正确做法 |
|---|---|---|
| 摄像头 | `SJ16/SJ26/SJ27` 需改焊 | 按 AN14191/板级手册图确认 A/B 侧；默认配置会干扰摄像头数据 |
| 摄像头 + Ethernet | 默认改焊会禁用 Ethernet | 需要按板级手册给出的飞线/焊桥变体验证，或改板 |
| 摄像头 + CAN | 共享 P1_10/P1_11 | 不能直接同时使用，需重新选引脚或改板 |
| MicroSD | 卡座 DNP | 装配卡座并核对共享 PWM 网络 |
| Pmod | J7 DNP | 装配后还要处理与 Arduino/摄像头/PDM 的复用 |
| Octal Flash | U7 DNP | 与 U8 封装重叠，需替换装配 |
| 5 V 扩展供电 | 与 USB/Arduino 入口并存 | 只保留一个主动电源，检查反向电流路径 |

## 8. 调试器与恢复

### 正常调试

1. J17 接 PC。
2. J18/J19 保持默认开路。
3. 目标 MCU 通过板载 MCU-Link SWD 调试。
4. D10 绿色 USB COMM 常亮，D11 红色心跳，D12 VCOM 活动。

### 外部调试器

1. 短接 J19，禁用板载 MCU-Link SWD。
2. 外部调试器接 J23。
3. J18 可保持开路以继续使用 VCOM。

### MCU-Link 固件异常

1. 断开 USB。
2. 短接 J21，重新接 USB，使 LPC55S69 进入 ISP。
3. 使用 NXP 固件更新工具重新写入 CMSIS-DAP 固件。
4. 断电，打开 J21，再上电。

## 9. 板级 errata

板级手册记录了带 `PMCXN947` 标记器件的若干问题，包括设备类型字段、外部 VDD 高于 3.5 V 时的 HVD、EdgeLock 2GO provisioning、唤醒时间和 `LDO_SYS` 外部电容不足。拿到板卡后必须检查芯片丝印：

- 若为 `PMCXN947`，在自定义电源、安全服务和低功耗实验中按 errata 处理；
- 若需要 EdgeLock 2GO，评估板不一定具备正式 provisioning；
- 量产设计和评审不能只依赖开发板现象。

## 10. 板卡使用检查清单

- [ ] 识别 MCXN947 芯片丝印与封装。
- [ ] 确认 J17 和 J11 用途，不把调试 USB 当目标 USB。
- [ ] 首次上电只接 J17，确认 D5 电源灯和 D10 USB COMM 灯。
- [ ] 保留 J18/J19/J21 默认状态。
- [ ] 记录要用的接口，逐项查原理图共享网络。
- [ ] 摄像头/Ethernet/CAN 组合先做互斥评审。
- [ ] 外接模块确认 3.3 V I/O 电平和最大供电电流。

