# USB、存储与文件系统

## 1. 两个 Type-C 不要混淆

| 接口 | MCU | 用途 |
|---|---|---|
| J17 | LPC55S69 MCU-Link | 调试、供电、VCOM、USBSIO |
| J11 | MCXN947 | 目标 MCU 的 USB HS Host/Device |

做 USB 应用必须接 J11；J17 是调试链路。两者同时接 PC 时，也要注意供电路径和主机枚举。

## 2. USB HS 能力

MCXN947 HS USB 提供：

- 片上 HS PHY；
- Host/Device；
- 可配合 VBUS 控制和检测；
- 板上有 NX5P3090 电源开关和 PTN5150A CC 逻辑。

软件路线：

1. 设备枚举；
2. CDC/VCOM；
3. HID；
4. MSC；
5. Audio；
6. 复合设备；
7. Host + 文件系统；
8. Type-C/PD 相关功能。

推荐示例路径：`examples/usb_examples/usb_device_cdc_vcom`、`usb_device_msc_disk`、`usb_host_msd_fatfs` 等。

### 物理检查

- D+/D- 差分阻抗；
- 共模扼流圈；
- ESD；
- VBUS 电流限制；
- CC 上下拉和角色配置；
- USB 主机/设备模式切换；
- 枚举时的浪涌和掉压。

## 3. 外部 QSPI Flash

默认器件：W25Q64JVSSIQ，8 MB。

- 控制器：FlexSPI；
- 信号：P3 端口；
- 可用于数据存储、资源、外部 XIP 等；
- 板上有重叠的 Octal Flash 封装，默认 DNP。

软件配置前确认：

- Flash 型号和命令集；
- 地址模式；
- dummy cycles；
- 读写时钟；
- 擦除块大小；
- 是否支持 XIP；
- 中断/DMA 访问；
- Flash 与代码缓存的交互。

普通内部 Flash 配置与 QSPI XIP 配置不是同一件事。需要明确选择 `hello_world_qspi_xip` 等专用配置。

## 4. MicroSD

J12 为 DNP，使用前需装配。uSDHC 相关引脚集中在 P2，部分与 Arduino PWM、摄像头共享。

开发顺序：

1. 卡检测；
2. 命令响应；
3. 单块读写；
4. 多块读写；
5. FatFs 挂载；
6. 掉电一致性；
7. 热插拔和坏卡。

## 5. 文件系统

SDK 提供：

- FatFs：适合 SD/USB 大容量存储，兼容 PC；
- LittleFS：适合片上/外部 NOR Flash，掉电一致性更好；
- 文件系统与 SDMMC、USB MSC、Block Device 适配层组合。

选择建议：

| 存储 | 推荐 |
|---|---|
| SD 卡/U 盘 | FatFs |
| 小型 NOR Flash 配置/日志 | LittleFS |
| 模型/固件镜像 | 自定义只读分区 + 校验 |

## 6. 掉电一致性

工业设备必须在任意时刻断电仍可恢复：

- 双区/日志式元数据；
- 写入顺序：数据 → 校验 → 提交标记；
- 上电扫描和回滚；
- 保留最小可用文件系统；
- 坏块/擦写计数；
- 写放大和寿命预算；
- 掉电测试至少覆盖每个写入阶段。

## 7. 推荐验收

- USB 插拔 1000 次枚举成功；
- Host/Device 角色切换稳定；
- USB 传输期间 MCU 不掉压；
- QSPI Flash 全空间读写校验通过；
- SD 卡异常拔出后文件系统可恢复；
- 断电测试无永久损坏；
- Flash 寿命满足产品每日写入预算。

