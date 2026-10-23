# Oscilloscope

FRDM-MCXN947 + LCD-PAR-S035 的双核示波器应用。`cm33_core0` 只负责 LVGL、
显示、触摸和共享数据可视化；`cm33_core1` 负责 ADC、eDMA、PowerQuad FFT。

## 当前功能

- ADC0_A0 单通道采样，输入接口为 Arduino A0，即 `J4` 引脚 2；
- 12-bit ADC，100 kS/s 硬件定时触发；
- 两只 1024 点缓冲，通过 eDMA TCD ping-pong 连续采集；
- 1024 点 PowerQuad 实数 FFT；
- 波形与频谱双视图；
- 峰峰值、峰值频率和 30 FPS 实时刷新；
- 中英文切换，默认中文；
- 采样错误、输入削顶和 DMA 缓冲溢出状态提示。

## 信号输入

直接把信号接到：

```text
J4 pin 2  ADC0_A0  Arduino A0
GND       开发板 GND
```

输入是单端 0-3.3 V 量程，信号必须以约 1.65 V 为直流中心。例如测试 1 kHz
正弦波时，建议使用：

```text
幅度:     1 Vpp
直流偏置: 1.65 V
```

禁止输入负电压或超过 3.3 V 的电压。当前版本没有前端衰减、交流耦合、过压保护
或抗混叠滤波。连接真实电路前应先完成输入保护设计，否则可能损坏 MCU。

## 采样参数

| 参数 | 值 |
|---|---:|
| ADC | ADC0，12-bit |
| 采样率 | 100 kS/s |
| 每缓冲点数 | 1024 |
| 单缓冲时间 | 10.24 ms |
| Nyquist 频率 | 50 kHz |
| FFT 分辨率 | 97.66 Hz |
| DMA | eDMA 双 TCD ping-pong |
| FFT | PowerQuad 1024 点 Q15 RFFT |
| UI | 30 FPS |

100 kS/s 是首版的吞吐、内存与频谱分辨率平衡点。若需要更高带宽，必须先增加
模拟抗混叠滤波，并重新评估 ADC 时序、FFT 周期和共享数据带宽。

## 双核边界

```text
cm33_core0
  LVGL 与显示刷新
  GT911 触摸
  ST7796S / FlexIO / TE / SmartDMA
  读取共享快照并绘制波形或频谱

cm33_core1
  ADC0 + LPTMR 触发
  eDMA ping-pong
  PowerQuad RFFT
  写入带 sequence 的共享快照
```

共享数据位于主/从核 linker 共同保留的 `rpmsg_sh_mem` 8 KiB 区域。写端先写奇数
sequence，写完后写偶数；读端只在前后 sequence 一致时接受数据。

## 构建与烧写

```bash
cd <repository-directory>
./apps/oscilloscope/build.sh
./apps/oscilloscope/build.sh --flash
```

构建使用 `west --sysbuild`：先构建 `cm33_core1`，生成 `core1_image.bin`，再把它
嵌入 `cm33_core0` 镜像。不要单独烧写 core1 ELF，否则会破坏主从核镜像布局。

## 中文字体

修改 `ui/ui_i18n.c` 的中文文案后运行：

```bash
./apps/oscilloscope/scripts/generate_fonts.sh
```

生成物位于 `ui/fonts/`。工程启用了 UTF-8、禁用 ASCII 编码，并启用压缩字形解码。

## 操作

- 顶部 `EN/中文`：切换界面语言；
- 底部主按钮：切换“波形”和“频谱”；
- 信息栏第一行：采样状态；
- 信息栏第二行：峰峰值和 FFT 峰值频率。

## 首版限制

- 无模拟前端，只有 0-3.3 V 单端输入；
- 无硬件抗混叠滤波器，超过 50 kHz 的信号会混叠；
- 三角窗和相对幅度归一化只用于首版频谱显示；
- 当前没有时基、垂直增益、触发沿、游标和波形存储；
- 频谱幅度是相对值，不是经过校准的 dBm；
- ST7796S 单 GRAM 在极快大面积更新时仍可能产生短暂撕裂。

## 实机验收

1. 不接信号时 UI 不重启，显示采集状态；
2. 输入 1.65 V DC，波形接近屏幕中线；
3. 输入 1 kHz、1 Vpp、偏置 1.65 V 正弦波，波形周期稳定；
4. 切换频谱后峰值应接近 1 kHz，允许约一个 97.66 Hz 频点误差；
5. 切换中英文，所有标签和信息栏同步更新；
6. 连续运行 30 分钟，无卡顿、无复位、状态栏不出现 DMA overrun；
7. 输入超过 0-3.3 V 边界时状态栏显示削顶告警。
