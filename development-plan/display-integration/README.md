# Display Selection and Integration

本目录针对 FRDM-MCXN947 的显示和触摸方案，先详细分析现有的 LCD-PAR-S035，再给出后续屏幕选型、硬件连接、驱动移植、LVGL 集成、点亮验收和量产风险控制方法。

## 当前结论

- LCD-PAR-S035 是 3.5 英寸、320×480、RGB565 输入、IPS 全视角、5 点电容触摸模块。
- 显示控制器是 ST7796U/ST7796S 系列，触摸控制器是 GT911。
- 它支持 8/16 位 8080 并口、3/4 线 SPI；在 FRDM-MCXN947 上优先使用 J8 的 16 位 8080 并口。
- MCUXpresso SDK 26.09 已包含 ST7796S、GT911 和 MCX FlexIO/SmartDMA LVGL 支持。
- 已实际编译 `lvgl_guider_bm` 成功：`../mcuxpresso-sdk/build/lvgl_s035_core0`。
- 屏幕、触摸、烧录和拖动功能已在 FRDM-MCXN947 上完成实机验证。

## 文档索引

| 文档 | 内容 |
|---|---|
| [01_lcd_par_s035_analysis.md](01_lcd_par_s035_analysis.md) | 面板、触摸、转接板、电源、接口、SDK 支持与限制 |
| [02_display_selection_guide.md](02_display_selection_guide.md) | 尺寸、分辨率、接口、内存、触摸、环境、生命周期与评分方法 |
| [03_frdm_mcxn947_hardware_integration.md](03_frdm_mcxn947_hardware_integration.md) | J8 并口、Pmod SPI、SW1、引脚、电源和背光 |
| [04_driver_porting_and_lvgl_integration.md](04_driver_porting_and_lvgl_integration.md) | ST7796S/GT911/FlexIO/SmartDMA/LVGL 的代码结构 |
| [05_power_on_touch_and_acceptance.md](05_power_on_touch_and_acceptance.md) | 从组装到显示、触摸、性能、稳定性验收 |
| [06_troubleshooting_and_production_risks.md](06_troubleshooting_and_production_risks.md) | 黑屏、花屏、方向、触摸、EMC、供应与生产问题 |
| [07_display_requirements_and_evaluation_template.md](07_display_requirements_and_evaluation_template.md) | 新项目选型表、接口预算和验收门槛模板 |
| [08_dual_core_ui_architecture.md](08_dual_core_ui_architecture.md) | Core0 UI 核与 Core1 功能核的职责、通信和资源规划 |

## 当前平台推荐

### 方案 A：LCD-PAR-S035 直接评估

```bash
source /home/jxy/NXP/MCXN947/development-plan/scripts/env.sh
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
- 当前已完成显示和触摸实机验证；快速拖动残余撕裂和双核迁移仍待继续优化。

