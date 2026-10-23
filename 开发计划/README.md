# FRDM-MCXN947 开发计划

本目录把开发工作拆成可独立执行的专题。建议先完成“环境 + 首次烧录”，再按目标产品选择外设、网络、AI、电机或安全路线，最后进入自制硬件与量产验证。

## 当前环境状态

| 项目 | 状态 | 位置或版本 |
|---|---|---|
| 主机 | 已验证 | WSL2，Ubuntu 24.04，内核 6.6.87.2 |
| Arm GNU Toolchain | 已安装并实际编译 | GCC 14.2.1，`../toolchains/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi` |
| MCUXpresso SDK | 已按目标板拉取 | 26.09.00，`../mcuxpresso-sdk` |
| west | 已安装 | 1.5.0，`../.venv` |
| CMake / Ninja | 已安装 | 3.31.10 / 1.13.2 |
| MCUXpresso for VS Code | 已安装 | 26.9.44 |
| LinkServer | 已解包并识别目标芯片 | 26.9.130，`../tools/linkserver/usr/local/LinkServer_26.9.130` |
| SDK 编译 | 已通过 | `hello_world` 生成 ELF/BIN |
| 开发板实机连接 | 未完成 | 当前未检测到 MCU-Link、串口或 USB 设备 |

当前工作目录约定：

```text
/home/jxy/NXP/MCXN947/
├── docs/                 原始及补充资料
├── mcuxpresso-sdk/       SDK 工作区和编译输出
├── toolchains/           Arm GCC
├── tools/linkserver/     本地解包的 LinkServer
├── .venv/                Python/west/CMake/Ninja 工具环境
└── 开发计划/             本开发计划
```

## 推荐阅读顺序

| 阶段 | 文档 | 目标 |
|---|---|---|
| 认识平台 | [01_资料阅读结论.md](01_资料阅读结论.md) | 知道什么资料解决什么问题 |
| 理解芯片 | [02_MCXN947芯片特性.md](02_MCXN947芯片特性.md) | 建立架构、存储、外设、安全边界 |
| 理解板卡 | [03_FRDM开发板特性与接口.md](03_FRDM开发板特性与接口.md) | 认识接口、跳线、冲突和默认 DNP |
| 选择方向 | [04_行业应用与产品方向.md](04_行业应用与产品方向.md) | 判断芯片是否适合目标产品 |
| 搭环境 | [05_WSL2命令行环境.md](05_WSL2命令行环境.md) | 复现已完成的工具链与 SDK |
| 首跑 | [06_Windows与VS_Code环境.md](06_Windows与VS_Code环境.md)、[07_首次烧录与串口验证.md](07_首次烧录与串口验证.md) | 板卡枚举、烧录、串口输出 |
| 做项目 | 08 至 18 | SDK、外设、网络、AI、电机、安全、双核 |
| 自开板 | 19 至 21 | 原理图、PCB、上电、Bring-up |
| 排障与进阶 | 22 至 24 | 故障定位、实验路线、资源索引 |

## 最短可执行路径

```bash
source /home/jxy/NXP/MCXN947/开发计划/scripts/env.sh
cd "$MCXN_SDK"

# 编译内部 Flash 版本的 hello_world
west build -b frdmmcxn947 \
  mcuxsdk/examples/demo_apps/hello_world \
  -d build/hello_world_core0 -p always \
  --config debug -Dcore_id=cm33_core0

# 连接 J17（MCU-Link USB-C）后烧录
west flash -d build/hello_world_core0 -r linkserver
```

串口参数通常为 `115200-8-N-1`。首次连接前还需要完成 WSL2 USB 直通或在 Windows 侧使用 MCUXpresso 扩展，详见第 05、06、07 篇。

## 重要边界

- 这块板适合评估、原型和软件先行开发，不等同于可直接量产的硬件设计。
- MCXN947 是通用工业 MCU，不应仅凭“有 CAN、Ethernet、电机外设”就宣称满足功能安全、汽车或医疗合规；合规需要器件等级、系统设计、认证和测试共同支撑。
- 摄像头与 Ethernet/CAN 在板上存在焊桥互斥；MicroSD、Pmod、外部 Octal Flash 默认未装配。
- 任何安全生命周期切换、熔丝编程、Mass Erase、掉电保持区改写都可能不可逆，先在可恢复的评估板上验证流程。

## 文档索引

| 编号 | 文档 |
|---|---|
| 00 | [00_开发路线总览.md](00_开发路线总览.md) |
| 01 | [01_资料阅读结论.md](01_资料阅读结论.md) |
| 02 | [02_MCXN947芯片特性.md](02_MCXN947芯片特性.md) |
| 03 | [03_FRDM开发板特性与接口.md](03_FRDM开发板特性与接口.md) |
| 04 | [04_行业应用与产品方向.md](04_行业应用与产品方向.md) |
| 05 | [05_WSL2命令行环境.md](05_WSL2命令行环境.md) |
| 06 | [06_Windows与VS_Code环境.md](06_Windows与VS_Code环境.md) |
| 07 | [07_首次烧录与串口验证.md](07_首次烧录与串口验证.md) |
| 08 | [08_SDK工程与构建系统.md](08_SDK工程与构建系统.md) |
| 09 | [09_时钟复位与电源低功耗.md](09_时钟复位与电源低功耗.md) |
| 10 | [10_GPIO定时器与PWM.md](10_GPIO定时器与PWM.md) |
| 11 | [11_串口SPI_I2C_I3C.md](11_串口SPI_I2C_I3C.md) |
| 12 | [12_CANFD与Ethernet.md](12_CANFD与Ethernet.md) |
| 13 | [13_USB存储与文件系统.md](13_USB存储与文件系统.md) |
| 14 | [14_FlexIO_显示_摄像头_音频_触摸.md](14_FlexIO_显示_摄像头_音频_触摸.md) |
| 15 | [15_EdgeAI_NPU与PowerQuad.md](15_EdgeAI_NPU与PowerQuad.md) |
| 16 | [16_电机控制与模拟信号链.md](16_电机控制与模拟信号链.md) |
| 17 | [17_TrustZone安全启动与升级.md](17_TrustZone安全启动与升级.md) |
| 18 | [18_双核开发与核间通信.md](18_双核开发与核间通信.md) |
| 19 | [19_自定义硬件设计指南.md](19_自定义硬件设计指南.md) |
| 20 | [20_引脚复用与冲突矩阵.md](20_引脚复用与冲突矩阵.md) |
| 21 | [21_硬件Bringup与测试计划.md](21_硬件Bringup与测试计划.md) |
| 22 | [22_故障排查手册.md](22_故障排查手册.md) |
| 23 | [23_学习路线与实验清单.md](23_学习路线与实验清单.md) |
| 24 | [24_参考资源与链接.md](24_参考资源与链接.md) |

## 专题子计划

| 目录 | 内容 |
|---|---|
| [屏幕选型与适配](屏幕选型与适配/README.md) | LCD-PAR-S035 详细分析、屏幕选型、FRDM 硬件连接、ST7796S/GT911/LVGL 集成、点亮验收和量产风险 |
| [触摸屏测试工程](../apps/touchscreen_tester/README.md) | 自由绘制、坏块检测、点击精度、设置页及性能验收 |

## 辅助脚本

| 脚本 | 用途 |
|---|---|
| `scripts/env.sh` | 为当前 shell 设置 SDK、GCC、west、LinkServer 路径 |
| `scripts/check_env.sh` | 检查工具版本、SDK 板级支持、构建产物和 USB/串口状态 |
| `scripts/build_hello.sh` | 重编并验证 `hello_world` |
| `scripts/monitor.sh` | 打开 115200 串口监视器，自动发现唯一串口 |
| `scripts/install_udev_rules.sh` | 需要 sudo 时安装 LinkServer 的 udev 规则 |
