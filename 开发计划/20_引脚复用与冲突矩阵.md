# 引脚复用与冲突矩阵

## 1. 为什么必须先做冲突表

MCXN947 的引脚复用层次包括：

- 芯片 pinmux 功能；
- 封装可用引脚；
- 板上 0 Ω/焊桥选择；
- Arduino/FRDM 内外排；
- mikroBUS/Pmod/FlexIO/摄像头共享；
- MCU-Link 调试桥共享；
- SD/Flash 选择。

“芯片支持”只代表功能存在，不代表开发板或产品板能同时连接。

## 2. 主板级关键共享

| 网络/功能 | 引脚 | 共享对象 | 结论 |
|---|---|---|---|
| RGB 红 | P0_10 | Arduino D9 | 点灯可能改变 Arduino 网络用途 |
| RGB 绿 | P0_27 | Arduino D10 / FC1 SPI PCS | 与 SPI 片选共享 |
| RGB 蓝 | P1_2 | Arduino D6 / MCU-Link SPI | 与调试桥和 CTimer 输出共享 |
| 触摸滑条 | P1_3 | mikroBUS RST / MCU-Link SPI PCS | 不宜同时启用所有功能 |
| I3C SDA | P1_16 | mikroBUS RX / 摄像头 D12 | 三选一/需改板 |
| I3C SCL | P1_17 | mikroBUS TX / 摄像头 D13 | 三选一/需改板 |
| CAN0 TX | P1_10 | 摄像头 D6 / 可接 Arduino J4 | 摄像头与 CAN 冲突 |
| CAN0 RX | P1_11 | 摄像头 D7 / I3C PUR | 摄像头与 CAN/I3C 相关网络冲突 |
| Ethernet | P1_4..P1_15 等 | 摄像头 D0-D11、时钟等 | 摄像头改焊默认影响 Ethernet |
| SDHC | P2_1..P2_7 | Arduino PWM / 摄像头 CLKIN | 默认卡座 DNP |

## 3. Arduino/FRDM 排针

### J1 关键项

| 引脚 | 网络/GPIO | 风险 |
|---|---|---|
| 2 | P4_3 / FC2 UART RX | 与 MCU-Link UART 共享 |
| 4 | P4_2 / FC2 UART TX | 与 MCU-Link UART 共享 |
| 7 | P1_21 SAI MCLK | 与 Ethernet MDIO 网络相关 |
| 8 | P1_23 / D3 | 与摄像头 LCD_RD |
| 10 | P0_30 / D4 | 可切换到编码器 |
| 12 | P1_21 / D5 | 或切到 SDHC PWM |
| 14 | P1_2 / D6 | 与蓝灯、SPI 桥、编码器相关 |
| 16 | P0_31 / D7 | 普通 GPIO |

### J2 关键项

| 引脚 | 网络/GPIO | 风险 |
|---|---|---|
| 3 | P0_14 或 P5_2 | 电流/BEMF 采样网络 |
| 4 | P0_10 / D9 | 与红灯共享 |
| 6 | P0_27 / D10 | 与绿灯、SPI PCS 共享 |
| 8 | P0_24 / D11 或 P2_2 PWM | SDHC/摄像头共享 |
| 10 | P0_26 / D12 | SPI MISO |
| 12 | P0_25 / D13 | SPI SCK |
| 15 | P1_1 TSI/I2C SCL | 与 mikroBUS I2C、MCU-Link SPI 相关 |
| 17 | P1_0 TSI/I2C SDA | 与 mikroBUS I2C、MCU-Link SPI 相关 |
| 18/20 | P4_0/P4_1 I2C | 或切到 I3C/摄像头 |

### J3

J3 主要提供 Arduino 电源、PWM、编码器、ADC 和复位。P2_2/P2_3/P2_4/P2_5/P2_6/P2_7 与 SDHC 和摄像头相关，不能默认与 SD 卡同时使用。

### J4

J4 包含：

- ADC0 输入；
- 电机电流/BEMF/运放网络；
- 触摸通道；
- 编码器和 USB 控制相关复用。

用于模拟输入时，务必核对对应 GPIO/触摸网络是否也被软件占用。

## 4. mikroBUS

### J6

- AN：ADC1_A0；
- RST：P1_3，与触摸和 MCU-Link SPI 相关；
- CS：P3_23；
- SCK：P3_21，与 Arduino SAI 网络；
- MISO：P3_22；
- MOSI：P3_20，与 Arduino SAI 网络。

### J5

- PWM：P3_19，与 Arduino 共享；
- INT：P5_7；
- RX：P1_16，与 I3C SDA、摄像头共享；
- TX：P1_17，与 I3C SCL、摄像头共享；
- SCL：P1_1；
- SDA：P1_0。

结论：使用 mikroBUS UART 时不能默认同时使用板载 I3C 传感器和摄像头对应通道。

## 5. Pmod J7

J7 默认 DNP。装配后关键网络：

- P0_19 CS；
- P0_20 INT；
- P0_16 MOSI；
- P5_6/P0_5 RESET/PDM DATA；
- P0_18 MISO；
- P3_3/P0_4 I2C/PDM CLK；
- P0_17 SPI CLK；
- P3_2 I2C SDA。

其中 P0_4/P0_5 与摄像头 VSYNC/PCLK 相关，P3_2/P3_3 与摄像头 SCCB 相关。

## 6. FlexIO/LCD J8

主要引脚：

- I2C：P4_1/P4_0；
- INT：P4_6；
- BLK：P4_5；
- RST：P4_7；
- D/C：P0_7；
- CS：P0_12；
- WR：P0_9；
- RD：P0_8；
- TE：P0_13；
- D0-D15：P2_8..P2_11、P4_12..P4_23。

冲突：

- P4_12/P4_13/P4_16/P4_20 与 Arduino 电机/采样网络共享；
- P4_5 可切到 Arduino J1；
- 所有显示模块必须使用 3.3 V I/O。

## 7. 摄像头 J9

摄像头使用：

- P1_4..P1_7：D0-D3；
- P3_4/P3_5：D4/D5；
- P1_10/P1_11：D6/D7；
- P0_5：PCLK；
- P2_2：CAMERA_CLKIN；
- P0_4：VSYNC；
- P0_11：HSYNC；
- P3_2/P3_3：SCCB I2C；
- P1_16/P1_17：部分显示/摄像头网络；
- P1_12..P1_15、P1_18/P1_19、P1_22/P1_23、P2_0：显示/摄像头控制。

### 摄像头改焊

- 默认需要调整 `SJ16`、`SJ26`、`SJ27`；
- AN14191 描述为 A 侧改到 B 侧；
- 默认改法禁用 Ethernet；
- 板级手册给出摄像头与 Ethernet 并存的特殊飞线/焊桥变体；
- CAN 的 P1_10/P1_11 也受影响；
- 改动必须记录、贴标签并重新测试网络功能。

## 8. 选择接口的方法

1. 列出所有需要同时运行的外设。
2. 从表中选择控制器实例。
3. 用 MCUXpresso Config Tools 找可用 pinmux。
4. 对照原理图查每个候选引脚的板级网络。
5. 标记 0 Ω/焊桥/DNP 和去焊风险。
6. 做外设并发矩阵。
7. 在自制板上重新分配引脚，不要受 FRDM 扩展接口限制。

## 9. 外设并发矩阵模板

| 外设 | 必需引脚 | 冲突外设 | 共存结论 | 证据 |
|---|---|---|---|---|
| UART0 VCOM | P1_8/P1_9 | UART ISP | 可共存 | 板级手册 |
| I3C1 温度 | P1_16/P1_17 | mikroBUS UART、摄像头 | 默认不共存 | 原理图 |
| CAN0 | P1_10/P1_11 | 摄像头 D6/D7 | 需要改焊/改板 | 原理图/AN |
| Ethernet | P1_4.. | 摄像头 | 默认摄像头改焊会禁用 | 板级手册 |
| SD | P2_1..P2_7 | Arduino PWM/摄像头 | 需选择 | 板级手册 |
| FlexIO LCD | P4/P2/P0 | Arduino 采样 | 部分冲突 | J8 pinout |

在完成此表之前，不应冻结产品引脚图。

