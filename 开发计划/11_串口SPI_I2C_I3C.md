# 串口、SPI、I2C 与 I3C

## 1. FlexComm

MCXN947 有 10 个低功耗 FlexComm，可按实例配置为：

- LPUART；
- LPSPI；
- LPI2C。

同一实例不能同时按三种协议使用。引脚复用、实例编号和板上网络必须一起确认。

## 2. LPUART

### 板上默认链路

- MCU-Link VCOM 到 `P1_8/P1_9`；
- J17 提供 USB 转串口；
- J18 必须开路；
- `P1_8/P1_9` 也是 UART ISP 引脚。

### 开发检查

- 波特率时钟来源；
- TX/RX 极性；
- 流控是否需要；
- DMA 接收和空闲中断；
- 调试日志是否阻塞实时任务；
- ISP 模式是否影响正常 UART。

推荐实验：`examples/driver_examples/lpuart/interrupt_transfer`、`lpuart/interrupt_rb_transfer`、`demo_apps/hello_world`。

## 3. LPSPI

主要检查：

- CPOL/CPHA；
- 主/从模式；
- PCS 片选；
- 位序；
- 时钟频率；
- 数据帧宽度；
- DMA 与片选保持。

板载 mikroBUS J6 使用 `P3_23/P3_21/P3_22/P3_20` 等 SPI 网络；这些引脚与 Arduino 或其它扩展存在共享，使用前必须查表。

Pmod J7 默认 DNP，装配后 SPI 信号包括 `P0_19/P0_16/P0_18/P0_17` 等。

## 4. LPI2C

检查：

- 7-bit/10-bit 地址；
- 上拉电压和阻值；
- 总线电容；
- 时钟拉伸；
- repeated START；
- 仲裁和总线恢复；
- 传感器上电时序。

不要在 1.8 V 和 3.3 V 设备之间直接接 I2C。使用电平转换或统一电源域。

## 5. I3C 与板载温度传感器

板载 P3T1755DP：

- I3C1；
- `P1_16` 数据、`P1_17` 时钟；
- I2C 地址 `0x48`；
- 可演示 I3C 动态地址、带内中断等能力；
- 数据线与 GPIO `P1_11/I3C1_PUR` 的上拉控制相关。

共享风险：

- `P1_16/P1_17` 同时连到 mikroBUS J5 的 RX/TX；
- 摄像头连接器也使用这两路；
- Arduino J2 有可选连接；
- CAN 使用 `P1_11`。

因此默认板上不能把 I3C 传感器、mikroBUS UART 和摄像头当成三个完全独立接口。

推荐实验：

1. I2C 兼容模式读取温度；
2. I3C 模式读取；
3. 温度告警；
4. 与 mikroBUS 模块共存性验证。

## 6. 物理层调试方法

### UART

- 先测 TX 静态高电平；
- 再测波特率；
- 最后看字节内容；
- 串口乱码优先比较时钟和波特率误差。

### SPI

- 示波器同时看 SCK、MOSI、MISO、CS；
- 检查 CS 前后是否多一个时钟；
- 检查 DMA 传输后 CS 是否释放。

### I2C/I3C

- 看 SDA/SCL 上拉和边沿；
- 确认 ACK/NACK；
- 检查总线是否被从设备拉住；
- 运行总线恢复流程。

## 7. 推荐验收

- UART 连续传输 1 MB 无丢字节；
- SPI 全双工回环和真实外设均通过；
- I2C 总线上电、热插拔和异常断开可恢复；
- I3C 设备发现、读写和告警稳定；
- DMA 接收在高速率下无覆盖；
- 所有信号幅值不超过对应 VDD_Px。

