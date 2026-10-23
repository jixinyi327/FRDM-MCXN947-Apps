# 屏幕选型与适配

本目录针对 FRDM-MCXN947 的显示和触摸方案，先详细分析现有的 LCD-PAR-S035，再给出后续屏幕选型、硬件连接、驱动移植、LVGL 集成、点亮验收和量产风险控制方法。

## 当前结论

- LCD-PAR-S035 是 3.5 英寸、320×480、RGB565 输入、IPS 全视角、5 点电容触摸模块。
- 显示控制器是 ST7796U/ST7796S 系列，触摸控制器是 GT911。
- 它支持 8/16 位 8080 并口、3/4 线 SPI；在 FRDM-MCXN947 上优先使用 J8 的 16 位 8080 并口。
- MCUXpresso SDK 26.09 已包含 ST7796S、GT911 和 MCX FlexIO/SmartDMA LVGL 支持。
- 已实际编译 `lvgl_guider_bm` 成功：`../mcuxpresso-sdk/build/lvgl_s035_core0`。
- 当前未连接屏幕实物，因此“编译成功”不等于“显示和触摸已验证”。

## 文档索引

| 文档 | 内容 |
|---|---|
| [01_LCD_PAR_S035详细分析.md](01_LCD_PAR_S035详细分析.md) | 面板、触摸、转接板、电源、接口、SDK 支持与限制 |
| [02_屏幕选型决策指南.md](02_屏幕选型决策指南.md) | 尺寸、分辨率、接口、内存、触摸、环境、生命周期与评分方法 |
| [03_FRDM_MCXN947硬件适配.md](03_FRDM_MCXN947硬件适配.md) | J8 并口、Pmod SPI、SW1、引脚、电源和背光 |
| [04_驱动移植与LVGL集成.md](04_驱动移植与LVGL集成.md) | ST7796S/GT911/FlexIO/SmartDMA/LVGL 的代码结构 |
| [05_点亮触摸与验收流程.md](05_点亮触摸与验收流程.md) | 从组装到显示、触摸、性能、稳定性验收 |
| [06_排障与量产风险.md](06_排障与量产风险.md) | 黑屏、花屏、方向、触摸、EMC、供应与生产问题 |
| [07_屏幕需求与评估模板.md](07_屏幕需求与评估模板.md) | 新项目选型表、接口预算和验收门槛模板 |

## 当前平台推荐

### 方案 A：LCD-PAR-S035 直接评估

```bash
source /home/jxy/NXP/MCXN947/开发计划/scripts/env.sh
cd "$MCXN_SDK"

west build -b frdmmcxn947 \
  mcuxsdk/examples/lvgl_examples/lvgl_guider_bm \
  -d build/lvgl_s035_core0 -p always \
  --config debug -Dcore_id=cm33_core0

west flash -d build/lvgl_s035_core0 -r linkserver
```

硬件上优先把屏幕转接板接到 FRDM-MCXN947 的 FlexIO/LCD J8，并按丝印将 SW1 设置为 16 位 8080 模式。

### 方案 B：SPI 屏幕

适合小尺寸、低刷新率、引脚预算紧张的产品。FRDM 的 Pmod J7 默认 DNP，而且 SPI 全屏吞吐明显低于 16 位并口。不要把 SPI 屏和 LCD-PAR-S035 的并口目标性能混为一谈。

### 方案 C：新屏幕

优先选择：

1. 有明确控制器型号和完整初始化序列；
2. 3.3 V I/O、已带升压/背光驱动或给出背光参数；
3. 有 MCUXpresso/LVGL 驱动基础；
4. 量产生命周期和货源稳定；
5. 按产品环境选择宽温、亮度、手套触摸和防护等级。

## 关键风险

- 面板额定工作温度为 `-20 °C` 至 `+70 °C`，不能直接用于宽温工业产品。
- 典型亮度约 380 cd/m²，不是户外阳光可读屏。
- 3.5 英寸 480×320 RGB565 单帧约 300 KiB，不能做完整双帧缓冲。
- 显示和触摸共用复位；GT911 地址与复位/INT 时序有关。
- 转接板存在 3.3 V 与 5 V/DC-DC 多种供电路径，必须确认只启用一种。
- 当前验证仅到编译级，实机显示、触摸和背光仍待连接屏幕后验收。

