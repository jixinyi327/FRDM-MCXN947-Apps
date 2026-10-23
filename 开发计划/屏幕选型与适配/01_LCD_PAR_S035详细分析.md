# LCD-PAR-S035 详细分析

## 1. 资料清单

| 文件 | 内容 |
|---|---|
| `LCD-PAR-S035/HP35001-DCT.pdf` | 面板、触摸、电气、光学、引脚、可靠性原始规格，16 页 |
| `LCD-PAR-S035/SPEC_HP35001-DCT_CN.pdf` | NXP 发布的面板规格副本，1 页摘要 |
| `LCD-PAR-S035/LCD-PAR-S035_A4.pdf` | 转接板原理图，Rev.A4 |
| `LCD-PAR-S035/LCD-PAR-S035_DESIGNFILES.zip` | KiCad 原理图/PCB、Gerber、BOM、坐标和 IPC 网表 |

原面板规格的版本为 `HP35001-DCT Rev.v0.0 / 2014 Feb`。这是一份较老的供应商规格，量产前必须重新确认面板版本、生命周期和当前批次参数。

## 2. 面板核心参数

| 参数 | 数值/说明 |
|---|---|
| 尺寸 | 3.5 英寸 |
| 显示技术 | Transmissive IPS，全视角 |
| 分辨率 | 320 RGB × 480，HVGA，纵向原生 |
| 推荐数据格式 | RGB565 |
| 最大颜色 | 产品页标称 262K 色 |
| 显示控制器 | ST7796U；SDK 驱动名为 ST7796S |
| 触摸控制器 | GT911 |
| 触摸 | 5 点电容触摸 |
| 接口 | 3/4 线 SPI、8/16 位 8080 并口 |
| 模组尺寸 | 55.26 × 84.52 × 3.83 mm，厚度不含 FPC |
| 有效显示区 | 48.96 × 73.44 mm |
| 像素尺寸 | 0.153 × 0.153 mm |
| 估算 PPI | 约 165 PPI |
| 对比度 | 典型 1000:1 |
| 响应时间 | 30 ms 典型，40 ms 最大 |
| 视角 | 四方向典型约 80° |
| 表面亮度 | 典型 380 cd/m² |
| 亮度均匀性 | ≥80% |
| 触摸透过率 | ≥85% |
| 表面硬度 | ≥6H |
| 工作温度 | `-20 °C` 至 `+70 °C`，≤90%RH |
| 存储温度 | `-30 °C` 至 `+80 °C`，≤90%RH |

## 3. 方向与分辨率

面板原生为 320 宽 × 480 高。MCUXpresso 的 LVGL 适配使用：

```c
#define LCD_WIDTH  480
#define LCD_HEIGHT 320
```

即通过 ST7796S 的 `kST7796S_Orientation270` 转为 480×320 横向显示。GT911 坐标同时做 X/Y 交换和 X 反向映射。

如果自定义 UI 改成竖屏或另一方向，必须同步修改：

- ST7796S orientation；
- GT911 坐标交换/反向；
- LVGL 分辨率；
- 图片和 UI 资源布局；
- 触摸校准和验收用例。

## 4. 接口模式

面板使用 IM2/IM1/IM0 选择模式：

| IM2 | IM1 | IM0 | 模式 | 数据线 |
|---:|---:|---:|---|---|
| 0 | 1 | 0 | 16 位 8080 | DB15–DB0 |
| 0 | 1 | 1 | 8 位 8080 | DB7–DB0 |
| 1 | 0 | 1 | 3 线 9-bit SPI | SDA 输入、SDO 输出 |
| 1 | 1 | 1 | 4 线 8-bit SPI | SDA 输入、SDO 输出 |

转接板 SW1 的物理位置与 IM2/IM1/IM0 对应。使用 J8 并口时要确认 SW1 处于 `010` 的 16 位 8080 档位，不能仅凭软件配置猜测。

## 5. 主要信号

### 显示端

- `VDDI`：IO 电源，1.8–3.3 V；
- `VDDA`：模拟升压电路，2.8–3.3 V；
- `VDDI` 与 `VDDA` 可直接接同一组 2.8–3.3 V；
- `CSX/SPI_CS`：片选；
- `DCX/SPI4_DC`：命令/数据选择；
- `WRX/SPI_SCLK`：并口写或 SPI 时钟；
- `RDX`：并口读；
- `DB0–DB15`：数据总线；
- `RESET`：低有效复位；
- `TE`： tearing effect 输出，本方案默认关闭；
- `SPI_MOSI/MISO`：SPI 输入/输出；
- `LED_A/LED_K`：背光阳极/阴极。

### 触摸端

- 3.3 V 供电；
- I2C SCL/SDA；
- INT；
- RESET；
- GT911 可由复位和 INT 时序选择 `0x5D` 或 `0x14` 的 7-bit 地址。

## 6. 转接板硬件

### 连接器

- J1：与 FRDM/MCX N 系列并行 LCD/FlexIO 排针配合；
- J5：2×6 Pmod，用于 SPI 模式；
- J2：8-pin 电容触摸 FPC；
- J4：32-pin LCD FPC；
- J3：Micro USB，主要用于独立供电/评估；
- J6：2×2 电源跳线头。

原理图显示 J1 图上标注为 `Header_02x14`，但当前 BOM 对应 `PinHeader_2x16`。这可能是原理图注释/版本遗留差异。装配和新项目导入前必须以 KiCad 源文件、PCB 封装、Pin 1 丝印和实物导通为准。

### 接口选择开关

SW1 同时决定显示接口，也是板上最容易被忽略的配置。换接口时：

1. 断电；
2. 设置 SW1；
3. 确认连接器也切换；
4. 重新编译对应驱动；
5. 测 D/C、CS、WR、数据线或 SPI 时钟。

### 背光

- 8 颗白光 LED 并联；
- 背光典型 3.1 V、120 mA；
- 转接板用 Q1 P-MOS 控制，电阻 R3 约 2.7 Ω 做限流；
- GPIO 可关断背光，具体有效电平以原理图和实测为准；
- 背光电流约占总功耗的主要部分。

### DC-DC

转接板有 M3406-ADJ、2.2 µH 电感和反馈分压，用于从 5 V/VBUS 产生本地约 3.3 V。FRDM 可直接由 J8 的 3.3 V 供电，也可使用独立 5 V 路径。

关键警告：

- 不要同时让 EVK 3.3 V 与板载 DC-DC 输出互相顶灌；
- 先用万用表确认 R5、R7、R9 等 0 Ω 选择的实际装配；
- 独立 USB 供电时不要再从 J8 注入 3.3 V；
- 首次上电用限流电源，先测 VCC/VDD 与背光电流。

### I2C 上拉

转接板有 4.7 kΩ 触摸 I2C 上拉。若目标主板上也有强上拉，需要合并计算：

```text
R_parallel = 1 / (1/R1 + 1/R2 + ...)
```

总线电容和上拉共同决定上升沿。过强上拉会增加漏电和功耗，过弱会导致高速下数据错误。

## 7. 供电与功耗预算

面板规格给出的 LCD 功耗典型 0.462 W，背光 120 mA。按 3.1–3.3 V 估算，显示模块总电流大致在 140–160 mA 量级，实际以测量为准。

产品设计建议先预留：

- 显示 + 触摸：至少 200 mA 的 3.3 V 裕量；
- 启动和白色满屏瞬间峰值；
- 5 V 输入到 3.3 V 的转换效率；
- 背光 PWM 时的脉冲电流；
- 面板、主控、通信同时工作时的电源纹波。

## 8. SDK 支持情况

MCUXpresso SDK 26.09 已包含：

- `components/display/st7796s`；
- `components/touch/gt911`；
- `examples/lvgl_examples/lvgl_sdk/lvgl_support/mcx_flexio`；
- `examples/lvgl_examples/lvgl_sdk/lvgl_support/flexcomm_spi_lcd_par_s035`；
- FRDM-MCXN947 的 `lvgl_examples` 板级适配。

FRDM-MCXN947 的已验证构建路径实际选择了 `mcx_flexio`：

```text
CONFIG_BOARD_USE_FLEXIO_SMARTDMA=y
CONFIG_LVGL_SUPPORT_VDB_COUNT=2
```

源码使用：

- FlexIO MCULCD 8080 模式；
- SmartDMA 搬运像素；
- ST7796S 驱动；
- GT911 驱动；
- LVGL 9.4.0；
- 480×320、RGB565；
- 两个约 1/10 屏高的局部缓冲。

已编译产物：

```text
build/lvgl_s035_core0/lvgl_guider_bm_cm33_core0.elf
build/lvgl_s035_core0/lvgl_guider_bm_cm33_core0.bin
```

构建结果中：

- Flash text 约 312,844 B；
- 静态 RAM 约 119,160 B；
- BIN 约 313,928 B。

这些数字会随 UI、字体、图片和 LVGL 配置变化。

## 9. 官方与开源生态

- NXP 产品页确认 SPI 或 8/16 位 8080、I2C 触摸、320×480、262K 色、5 点触摸。
- Zephyr 提供 `lcd_par_s035_8080` 与 `lcd_par_s035_spi` 两种 shield。
- Zephyr SPI overlay 将 ST7796S 最大频率设为 30 MHz，并采用半双工写模式；这是面板/控制器理论允许值，不代表任何 MCU 都能稳定达到。
- NXP 官方 LVGL 当前路线采用 16 位 FlexIO 并口，说明该屏在 MCXN947 上的首选不是 SPI。

## 10. 优势

- 3.3 V 逻辑与 FRDM 兼容；
- 并口模式吞吐明显高于 SPI；
- ST7796S 和 GT911 都是成熟控制器，驱动资料多；
- IPS 全视角；
- 5 点触摸；
- 有原理图、PCB、BOM 和 Gerber；
- NXP SDK 和 Zephyr 均有现成支持。

## 11. 限制

- 工作温度仅到 `+70 °C`；
- 亮度约 380 cd/m²，户外可读性有限；
- 480×320 单帧约占 300 KiB SRAM；
- 32-pin FPC 和转接板增加机械与成本复杂度；
- 显示和触摸共用复位，调试顺序敏感；
- 供应商面板规格较老，长期供货需重新确认；
- 当前尚未进行实物显示、触摸和 EMC 验证。

## 12. 结论

LCD-PAR-S035 非常适合 MCXN947 的第一块彩色触摸屏和工业 HMI 原型，最佳路径是：

```text
FRDM J8 -> 16 位 8080 -> FlexIO -> SmartDMA
-> ST7796S -> LVGL
```

触摸：

```text
P4_0/P4_1 I2C2 -> GT911 -> P4_6 INT
```

它不适合直接用于宽温、强光、高可靠性或大屏产品；这些场景应重新选屏。

