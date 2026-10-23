# 驱动移植与 LVGL 集成

## 1. 当前 SDK 支持结构

MCUXpresso SDK 26.09 中，与这块屏相关的主要代码：

| 层 | 路径/组件 | 作用 |
|---|---|---|
| 显示控制器 | `components/display/st7796s` | 初始化、方向、区域、写像素 |
| 触摸控制器 | `components/touch/gt911` | 复位、地址、单点/多点读取 |
| LVGL 平台层 | `examples/lvgl_examples/lvgl_sdk/lvgl_support/mcx_flexio` | FlexIO 8080、SmartDMA、flush、touch 映射 |
| 板级引脚 | `examples/_boards/frdmmcxn947/lvgl_examples/pin_mux.c` | LCD 和触摸引脚 |
| 板级中断 | `examples/_boards/frdmmcxn947/lvgl_examples/hardware_init.c` | GT911 INT、I2C 总线恢复 |
| GUI 示例 | `examples/lvgl_examples/lvgl_guider_bm` | GUI Guider 生成界面的裸机示例 |

FRDM-MCXN947 当前实际选中的不是 `flexcomm_spi_lcd_par_s035`，而是 `mcx_flexio`。

## 2. 已有构建验证

本机已成功执行：

```bash
source /home/jxy/NXP/MCXN947/开发计划/scripts/env.sh
cd "$MCXN_SDK"

west build -b frdmmcxn947 \
  mcuxsdk/examples/lvgl_examples/lvgl_guider_bm \
  -d build/lvgl_s035_core0 -p always \
  --config debug -Dcore_id=cm33_core0
```

关键配置：

```text
CONFIG_BOARD_USE_FLEXIO_SMARTDMA=y
CONFIG_LVGL_SUPPORT_VDB_COUNT=2
CONFIG_LVGL_VERSION_MAJOR=9
CONFIG_LVGL_VERSION_MINOR=4
CONFIG_LVGL_VERSION_PATCH=0
```

这一结果证明：

- ST7796S/GT911 代码可编译；
- FlexIO 8080 驱动可编译；
- SmartDMA 路径可编译；
- LVGL 9.4 与当前 GCC 14.2.1 可构建；
- 链接无 Flash/RAM 溢出。

它尚未证明：

- SW1 档位正确；
- 背光点亮；
- 显示方向和颜色正确；
- GT911 能响应；
- 触摸坐标正确；
- 实际帧率和 EMC 合格。

## 3. 初始化流程

当前 `mcx_flexio` 平台层大致按以下顺序：

1. 初始化 LCD 时钟和 FlexIO 时钟；
2. 配置 FlexIO MCULCD 为 8080；
3. 初始化 CS、D/C、RST GPIO；
4. 初始化 SmartDMA/EDMA 数据搬运；
5. 拉低共用 RESET，延时，释放；
6. 用 ST7796S preset 初始化面板；
7. 清屏；
8. 初始化 GT911 的 I2C 和 INT；
9. 创建 LVGL display 和 input device；
10. 进入 UI 循环。

### 共用复位

显示控制器和 GT911 共用 RESET。当前实现不会在触摸初始化时再次单独复位屏幕，而是在 LCD 初始化阶段统一复位。

自定义代码若又单独复位 GT911，可能造成：

- LCD 被意外复位；
- GT911 地址切换；
- I2C 无响应；
- 触摸坐标丢失。

## 4. GT911 地址

GT911 支持：

- 7-bit 地址 `0x5D`；
- 7-bit 地址 `0x14`。

地址由复位时 INT 电平决定。当前 MCUXpresso 适配使用 `kGT911_I2cAddrAny`，驱动会尝试可用地址；Zephyr 的 LCD-PAR-S035 支持在复位时把 INT 拉低以选择 `0x5D`。

调试建议：

```text
先扫描 0x5D
再扫描 0x14
确认 INT 和 RESET 时序
不要只修改代码中的地址常量
```

## 5. 显示方向和触摸映射

当前配置：

```c
.orientationMode = kST7796S_Orientation270
.pixelFormat = kST7796S_PixelFormatRGB565
```

LVGL 分辨率：

```c
LCD_WIDTH  = 480
LCD_HEIGHT = 320
```

触摸坐标映射：

```c
data->point.x = tp[0].y;
data->point.y = touchResolutionX - tp[0].x;
```

若显示方向与触摸不一致，不要只旋转 UI；应同时校正 ST7796S MADCTL 和 GT911 映射。

## 6. 帧缓冲

当前平台层使用：

```text
render mode = LV_DISPLAY_RENDER_MODE_PARTIAL
buffer height = LCD_HEIGHT / 10
buffer count = 2
RGB565 = 2 bytes/pixel
```

每块缓冲：

```text
480 × 32 × 2 = 30,720 B
```

双缓冲：

```text
61,440 B
```

因此不能使用整帧双缓冲。

### Flush 流程

1. LVGL 只重绘变化区域；
2. 将 RGB565 数据写入局部缓冲；
3. `ST7796S_SelectArea()` 设置窗口；
4. SmartDMA/EDMA 发送像素；
5. 完成回调释放缓冲；
6. 调用 `lv_disp_flush_ready()`。

不要在 flush 内执行阻塞串口打印，否则会显著降低 UI 帧率。

## 7. LVGL 配置

当前板级配置包含：

```text
LV_MEM_SIZE_KILOBYTES=48
```

调整时要考虑：

- 屏幕分辨率；
- 字体数量和字号；
- 图片格式和压缩；
- 动画层数；
- 触摸队列；
- UI 树深度；
- 网络/视觉任务占用。

### 优化顺序

1. 先减少全屏重绘；
2. 使用局部刷新；
3. 减少大字号中文字库；
4. 图片放外部 Flash；
5. 使用合适的颜色格式；
6. 最后再增加 draw buffer 或 LVGL heap。

## 8. UI 工程组织

建议：

```text
ui/
├── generated/          GUI Guider 生成代码
├── custom/             事件和业务绑定
├── assets/             图片、字体、主题
└── port/               display/touch tick 和初始化
app/
├── business/           业务状态机
└── services/           网络、存储、AI
```

GUI Guider 重新生成时不要覆盖手写业务代码。把事件处理放到 `custom/` 或独立服务层。

## 9. 从示例生成产品工程

推荐在 MCUXpresso for VS Code 中：

1. 导入 `lvgl_guider_bm` 或最接近的 LVGL 示例；
2. 选择 FRDM-MCXN947；
3. 选择 Freestanding 应用；
4. 保存到独立项目目录；
5. 固定 SDK/工具链；
6. 把屏幕初始化和业务代码分层；
7. 提交版本库。

不要直接在共享 SDK 示例中做长期产品开发。

## 10. SPI 适配路线

若目标板必须用 SPI：

1. SW1 设为 4-wire SPI；
2. 连接 Pmod J5 到主控 SPI；
3. 使用 4-wire DBI/SPI DMA 实现；
4. 屏幕侧只做写时可采用半双工；
5. 从 5–10 MHz 起测，逐步升到面板/主控稳定上限；
6. 触摸仍用 I2C；
7. 采用局部刷新；
8. 测量全屏刷新时间和 CPU 占用。

Zephyr 的 S035 SPI overlay 采用最高 30 MHz，并注明读取过快的限制；不要直接从 30 MHz 开始 debug。

SDK 中存在 `flexcomm_spi_lcd_par_s035` 平台层，但当前 FRDM-MCXN947 默认构建选择的是 FlexIO 8080。移植时应核对：

- `BOARD_LCD_SPI`；
- SPI 实例/时钟；
- CS、DC、RST；
- DMA 请求和通道；
- touch I2C；
- 板级宏是否完整。

## 11. 移植到新屏幕

需要替换：

1. 显示控制器驱动；
2. 初始化序列；
3. 分辨率；
4. 颜色格式；
5. MADCTL 方向；
6. 窗口和写像素命令；
7. 触摸控制器驱动；
8. 触摸坐标映射；
9. LVGL 缓冲大小；
10. 背光和电源控制。

建议保留现有 `display -> DBI -> FlexIO/SPI` 分层，只替换控制器和板级接口，不把显示命令散落在 UI 代码。

## 12. 代码验收

- 初始化函数返回明确错误码；
- I2C/SPI 错误能恢复；
- 触摸中断丢失可检测；
- flush 不无限等待；
- SmartDMA/EDMA 完成回调可超时；
- 颜色/方向配置集中管理；
- 屏幕参数不散落魔法数字；
- 不同面板变体通过配置选择；
- 编译产物记录 SDK、GCC、LVGL 和 GUI Guider 版本。

