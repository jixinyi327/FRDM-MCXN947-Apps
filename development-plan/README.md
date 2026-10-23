# FRDM-MCXN947 Development Plan

本目录把开发工作拆成可独立执行的专题。建议先完成“环境 + 首次烧录”，再按目标产品选择外设、网络、AI、电机或安全路线，最后进入自制硬件与量产验证。

## 当前环境状态

| 项目 | 状态 | 位置或版本 |
|---|---|---|
| 主机 | 已验证 | WSL2，Ubuntu 24.04，内核 6.6.87.2 |
| Arm GNU Toolchain | 已安装并实际编译 | GCC 14.2.1，`../toolchains/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi` |
| Node.js | 已安装 | 24.22.1，`../toolchains/node-v24.22.1-linux-x64` |
| lv_font_conv | 已安装 | 1.5.3，`../tools/lv_font_conv` |
| MCUXpresso SDK | 已按目标板拉取 | 26.09.00，`../mcuxpresso-sdk` |
| west | 已安装 | 1.5.0，`../.venv` |
| CMake / Ninja | 已安装 | 3.31.10 / 1.13.2 |
| MCUXpresso for VS Code | 已安装 | 26.9.44 |
| LinkServer | 已解包并识别目标芯片 | 26.9.130，`../tools/linkserver/usr/local/LinkServer_26.9.130` |
| SDK 编译 | 已通过 | `hello_world` 生成 ELF/BIN |
| 开发板实机连接 | 已验证 | MCU-Link 探针可识别，显示、触摸和烧录测试见应用说明 |

当前工作目录约定：

```text
<repository-directory>/
├── docs/                 原始及补充资料
├── mcuxpresso-sdk/       SDK 工作区和编译输出
├── toolchains/           Arm GCC、Node.js 和固定工具链
├── tools/                LinkServer 与字体生成器
├── .venv/                Python/west/CMake/Ninja 工具环境
└── development-plan/     本开发计划
```

## 推荐阅读顺序

| 阶段 | 文档 | 目标 |
|---|---|---|
| 认识平台 | [01_source_reading_summary.md](01_source_reading_summary.md) | 知道什么资料解决什么问题 |
| 理解芯片 | [02_mcxn947_chip_features.md](02_mcxn947_chip_features.md) | 建立架构、存储、外设、安全边界 |
| 理解板卡 | [03_frdm_board_features_and_interfaces.md](03_frdm_board_features_and_interfaces.md) | 认识接口、跳线、冲突和默认 DNP |
| 选择方向 | [04_industry_applications_and_products.md](04_industry_applications_and_products.md) | 判断芯片是否适合目标产品 |
| 搭环境 | [05_wsl2_command_line_environment.md](05_wsl2_command_line_environment.md) | 复现已完成的工具链与 SDK |
| 首跑 | [06_windows_and_vscode_environment.md](06_windows_and_vscode_environment.md)、[07_first_flash_and_serial_validation.md](07_first_flash_and_serial_validation.md) | 板卡枚举、烧录、串口输出 |
| 做项目 | 08 至 18 | SDK、外设、网络、AI、电机、安全、双核 |
| 自开板 | 19 至 21 | 原理图、PCB、上电、Bring-up |
| 排障与进阶 | 22 至 24 | 故障定位、实验路线、资源索引 |

## 最短可执行路径

```bash
source development-plan/scripts/env.sh
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
| 00 | [00_development_roadmap.md](00_development_roadmap.md) |
| 01 | [01_source_reading_summary.md](01_source_reading_summary.md) |
| 02 | [02_mcxn947_chip_features.md](02_mcxn947_chip_features.md) |
| 03 | [03_frdm_board_features_and_interfaces.md](03_frdm_board_features_and_interfaces.md) |
| 04 | [04_industry_applications_and_products.md](04_industry_applications_and_products.md) |
| 05 | [05_wsl2_command_line_environment.md](05_wsl2_command_line_environment.md) |
| 06 | [06_windows_and_vscode_environment.md](06_windows_and_vscode_environment.md) |
| 07 | [07_first_flash_and_serial_validation.md](07_first_flash_and_serial_validation.md) |
| 08 | [08_sdk_projects_and_build_system.md](08_sdk_projects_and_build_system.md) |
| 09 | [09_clock_reset_power_and_low_power.md](09_clock_reset_power_and_low_power.md) |
| 10 | [10_gpio_timers_and_pwm.md](10_gpio_timers_and_pwm.md) |
| 11 | [11_uart_spi_i2c_i3c.md](11_uart_spi_i2c_i3c.md) |
| 12 | [12_canfd_and_ethernet.md](12_canfd_and_ethernet.md) |
| 13 | [13_usb_storage_and_filesystem.md](13_usb_storage_and_filesystem.md) |
| 14 | [14_flexio_display_camera_audio_touch.md](14_flexio_display_camera_audio_touch.md) |
| 15 | [15_edgeai_npu_and_powerquad.md](15_edgeai_npu_and_powerquad.md) |
| 16 | [16_motor_control_and_analog_signal_chain.md](16_motor_control_and_analog_signal_chain.md) |
| 17 | [17_trustzone_secure_boot_and_update.md](17_trustzone_secure_boot_and_update.md) |
| 18 | [18_dual_core_and_ipc.md](18_dual_core_and_ipc.md) |
| 19 | [19_custom_hardware_design_guide.md](19_custom_hardware_design_guide.md) |
| 20 | [20_pin_mux_and_conflict_matrix.md](20_pin_mux_and_conflict_matrix.md) |
| 21 | [21_hardware_bringup_and_test_plan.md](21_hardware_bringup_and_test_plan.md) |
| 22 | [22_troubleshooting_guide.md](22_troubleshooting_guide.md) |
| 23 | [23_learning_path_and_labs.md](23_learning_path_and_labs.md) |
| 24 | [24_reference_resources.md](24_reference_resources.md) |

## 专题子计划

| 目录 | 内容 |
|---|---|
| [display-integration](display-integration/README.md) | LCD-PAR-S035 详细分析、屏幕选型、FRDM 硬件连接、ST7796S/GT911/LVGL 集成、点亮验收和量产风险 |
| [触摸屏测试工程](../apps/touchscreen_tester/README.md) | 自由绘制、坏块检测、点击精度、设置页及性能验收 |

## 辅助脚本

| 脚本 | 用途 |
|---|---|
| `scripts/env.sh` | 为当前 shell 设置 SDK、GCC、west、LinkServer 路径 |
| `scripts/manage_dev_env.sh` | 检查/搭建 Linux 环境，并通过 SSH/HTTPS 检查或更新 SDK |
| `scripts/check_naming.sh` | 检查所有已跟踪或待提交路径是否使用 ASCII 英文命名 |
| `scripts/check_env.sh` | 检查工具版本、SDK 板级支持、构建产物和 USB/串口状态 |
| `scripts/build_hello.sh` | 重编并验证 `hello_world` |
| `scripts/monitor.sh` | 打开 115200 串口监视器，自动发现唯一串口 |
| `scripts/install_udev_rules.sh` | 需要 sudo 时安装 LinkServer 的 udev 规则 |
