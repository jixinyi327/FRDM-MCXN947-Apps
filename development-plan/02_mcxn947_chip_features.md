# MCXN947 芯片特性

## 1. 定位与器件型号

FRDM-MCXN947 搭载 MCXN947VDFT，属于 MCX N94x 系列。`V` 表示 `-40 °C` 至 `+125 °C` 结温等级，`DF` 表示 184 球、0.5 mm 间距、9 mm × 9 mm 的 VFBGA 封装，`T` 表示托盘包装。最终采购时应以最新数据手册的 Ordering Information 为准。

MCXN947 面向高性能工业 MCU：双 Cortex-M33、NPU、PowerQuad DSP、SmartDMA、丰富模拟/电机外设、工业有线通信和 EdgeLock 安全。

## 2. 核心计算资源

| 资源 | 规格 | 开发含义 |
|---|---|---|
| Cortex-M33 主核 | 150 MHz | 主应用、控制、通信、安全服务 |
| Cortex-M33 副核 | 150 MHz | 实时/隔离任务、通信卸载、双核协作 |
| CoreMark | 每核约 618，4.12 CoreMark/MHz | 不以整数吞吐替代 DSP/NPU 评估 |
| TrustZone | 支持 | 可划分安全与非安全世界 |
| FPU/SIMD | 支持 | DSP 与控制算法更有效，但功耗与时序仍须实测 |
| eIQ Neutron N1-16 NPU | 约 4.8 GOP/s | 适合量化后的小型视觉、语音、时序模型 |
| PowerQuad | DSP 协处理器 | FFT、滤波、矩阵等定点 DSP 运算 |
| SmartDMA | 可编程轻量协处理器 | 摄像头、键盘扫描、LCD 等并行/周期搬运 |
| PLC 控制器 | 支持 | 电力线通信相关应用 |

## 3. 存储资源

| 存储 | 规格 | 注意事项 |
|---|---|---|
| 片上 Flash | 最高 2 MB，双 1 MB Bank | 支持 Swap、Read While Write、ECC |
| 片上 SRAM | 最高 512 KB | 可配置为最高约 416 KB 带 ECC；ECC 和保留区会减少可用容量 |
| ECC RAM | 32 KB 独立 ECC SRAM | 可用于高可靠数据 |
| VBAT 保持 RAM | 最多 4 × 8 KB | 深掉电时保留关键状态 |
| ROM | 256 KB | 含安全启动/引导代码，应用不可用作普通 Flash |
| 外部存储器 | FlexSPI，16 KB Cache | QSPI/Octal SPI、HyperFlash/HyperRAM/Xccela 等，具体板级按封装与布线 |

做 AI、摄像头、音频和网络时必须同时估算：

```text
代码 Flash + 常量/模型 + 静态 RAM + 栈 + 堆
+ 网络/文件系统缓存 + DMA 缓冲 + 帧缓冲 + 双核共享内存
<= 器件可用资源 - ECC/保留区 - 安全隔离区
```

## 4. 连接与工业接口

| 接口 | MCXN947 能力 | 板级默认情况 |
|---|---|---|
| FlexComm | 10 个，每个可配置 LPUART/LPSPI/LPI2C | 部分被 MCU-Link、mikroBUS、Arduino 占用 |
| I3C | 2 个，兼容 I2C | I3C1 接 P3T1755DP 温度传感器 |
| USB HS | 1 个，带片上 HS PHY，Host/Device | 接 J11 USB-C |
| USB FS | 1 个，带片上 FS PHY | 芯片支持，FRDM 板未提供独立 FS 连接器 |
| FlexCAN | 2 个，支持 CAN FD | 板上默认使用 CAN0，经 TJA1057 |
| Ethernet | 1 个 10/100 Mbps，QoS | LAN8741A RMII PHY + RJ45 |
| uSDHC | 1 个 | 卡座 J12 默认 DNP |
| Smart Card | 2 个 EMVSIM | 需外部接口 |
| PLC | 1 个控制器 | 需外部模拟前端/耦合网络 |

## 5. 模拟与电机控制

| 模块 | 规格 | 适用场景 |
|---|---|---|
| ADC | 2 × 16-bit；4 路并行转换；16-bit 最高 2 Msps，12-bit 最高 3.15 Msps | 电机相电流、母线电压、传感器 |
| ADC 通道 | 最多 75 路，随封装变化 | 184BGA 可用数量应按 pinout 核对 |
| DAC | 2 × 12-bit/1 Msps，1 × 14-bit/5 Msps | 波形、偏置、模拟控制 |
| 比较器 | 3 个，含 8-bit DAC 参考 | 过流、过压、快速故障 |
| OpAmp | 3 个 | PGA、差分、仪表、跨导配置 |
| VREF | 约 1.0 V，±0.2%，15 ppm/°C | 高精度测量 |
| FlexPWM | 2 个实例，每实例 4 子模块/12 输出 | 三相逆变、互补 PWM、死区、故障关断 |
| QDC | 2 个 | 编码器/位置控制 |
| SINC | 3 阶、5 通道 | Σ-Δ 电流/电压采样 |
| EVTG/PLU/XBAR | 事件、逻辑和触发互联 | 少 CPU 干预的故障和同步路径 |

## 6. HMI、音频与视觉

- TSI：最多 25 个自电容通道，最多 8 TX × 17 RX 互电容通道；自电容模式下支持防水。
- FlexIO：可构造并行/串行接口、LCD 总线和摄像头时序。
- SAI：最多 4 通道，适合 I2S/TDM 音频。
- MICFIL/PDM：最多 4 个数字 MEMS 麦克风。
- 显示：板载 FlexIO/LCD 连接器，可接 `LCD-PAR-S035` 等 3.3 V I/O 模块。
- 摄像头：板载 SmartDMA/摄像头连接器，NXP 示例验证过 OV7670。

## 7. 安全能力

- EdgeLock Secure Enclave，Core Profile；
- AES-256、SHA-2、ECC P-256、TRNG、密钥生成/派生；
- PUF 设备唯一身份、DICE 设备证明；
- 不可变 ROM 安全启动、SB3 镜像、安全恢复和更新；
- ECDSA P-256/P-384 安全启动；
- 片上 Flash 实时加密和附加认证；
- 8 个主动/被动防拆引脚，电压、温度、光照、时钟和毛刺检测；
- TrustZone、TRDC 资源域控制、安全调试生命周期。

这些是“提供安全能力”，不等于产品自动满足某项法规或认证。密钥生命周期、调试口管理、生产环境、漏洞响应和升级签名必须形成完整制度。

## 8. 功耗和电气边界

数据手册给出的典型指标包括：

- 低至约 57 µA/MHz 的 Active 电流；
- Deep Sleep：约 170 µA，保留 512 KB SRAM；
- Power Down：约 5.2 µA，保留 512 KB SRAM；
- Deep Power Down：低至约 2.0 µA，RTC 和 32 KB RAM 保留，典型唤醒约 5.3 ms；
- VBAT 模式可保留最多 4 × 8 KB ECC RAM。

系统设计不能只看这些典型值，还要加外部传感器、PHY、Flash、LED、LDO/DCDC 损耗和上拉漏电。

主要工作电压边界：

| 电源域 | 工作范围/关系 |
|---|---|
| VDD_SYS | 1.71–1.98 V（正常），熔丝编程可到约 2.75 V |
| VDD_CORE | 由 DCDC 或 LDO 产生，绝对最大 1.26 V |
| VDD、VDD_P2/P3/P4 | 1.71–3.6 V；P2/P3 也支持 1.14–1.32 V 的受限模式 |
| VDD_BAT | 1.71–3.6 V |
| VDD_ANA | 必须与 VDD_P4 同压，偏差仅 ±0.1 V |
| VDD_USB | 3.0–3.6 V |
| 单数字输出引脚 | 最大 ±25 mA；整颗电源电流上限按各电源脚约束 |

上电顺序必须遵守数据手册：VDD_BAT 不晚于 VDD_SYS；次级 IO 或与 VDD 相连，或在 VDD_SYS 之后上电；VDD_CORE 在 VDD 之后；VDD_P4 与 VDD_ANA 同压。详见第 09、19 篇。

## 9. 封装、温度和可靠性

- 184 VFBGA：0.5 mm pitch，适合高密度设计，但对 fanout、过孔、回流焊和 X-ray 检测要求高。
- 芯片结温等级可到 `+125 °C`；FRDM 板级手册给出的整板验证范围为 `+105 °C`。
- Flash ECC 可做 1-bit 纠正、2-bit 检测；SRAM ECC 使用时会降低最大可用 RAM。
- 需要关注 Flash 擦写寿命、数据保持、DCDC 效率、热阻和长期高温老化。

## 10. 选型判断

适合 MCXN947 的条件：

- 需要 Cortex-M 实时性，同时需要中小型 NPU/DSP 加速；
- 需要 Ethernet、CAN FD、USB、模拟采集和电机控制中的多项；
- 需要安全启动、设备身份、安全升级；
- 能接受 512 KB 级 SRAM、10/100 Ethernet 和 BGA 封装。

可能不适合的条件：

- 需要千兆网络、Linux、大模型或大容量视频缓存；
- 需要车规器件或现成功能安全认证；
- 只需要极低成本和极少外设；
- 产品团队无法处理 0.5 mm BGA 的制造、返修与测试。

