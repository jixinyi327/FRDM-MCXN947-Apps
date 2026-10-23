# SDK 工程与构建系统

## 1. SDK 26.09 的仓库结构

当前工作区不是“所有文件都在一个 SDK zip”里，而是 west 管理的多仓库结构：

```text
mcuxpresso-sdk/
├── .west/
├── manifests/                     SDK 版本和仓库清单
└── mcuxsdk/
    ├── devices/                   芯片启动、系统、链接脚本
    ├── drivers/                   通用外设驱动
    ├── components/                板级驱动和通用组件
    ├── middleware/                RTOS、网络、USB、安全、AI 等
    ├── examples/                  通用应用源码和 example.yml
    ├── examples/_boards/          板级适配和配置
    ├── docs/                      SDK 文档源
    └── tool_data/                 芯片/板卡元数据
```

理解“通用示例代码”和“板级适配”分离非常重要：

```text
examples/demo_apps/hello_world/                     应用源码
examples/_boards/frdmmcxn947/demo_apps/hello_world/ 板级配置
examples/_boards/frdmmcxn947/cm33_core0/            核心配置
```

通用源码提供业务逻辑；`_boards/frdmmcxn947` 提供引脚、时钟、外设配置和板级说明。构建系统按 board/core 把两层组合起来。

## 2. 当前板级构建命令

```bash
source development-plan/scripts/env.sh
cd "$MCXN_SDK"

west build -b frdmmcxn947 \
  mcuxsdk/examples/demo_apps/hello_world \
  -d build/hello_world_core0 \
  -p always --config debug \
  -Dcore_id=cm33_core0
```

参数解释：

| 参数 | 含义 |
|---|---|
| `-b frdmmcxn947` | 板卡 ID，不带 core 后缀 |
| `-Dcore_id=cm33_core0` | 选择主核；副核用 `cm33_core1` |
| `-d` | 构建目录 |
| `-p always` | pristine，清理并重新生成 |
| `--config debug` | Debug 配置；也常用 `release` |

## 3. 查找可用示例

列出该板所有可用 ARM GCC 配置：

```bash
west list_project -p 'examples/*' \
  -b frdmmcxn947@cm33_core0 -t armgcc
```

只列一种目标：

```bash
west list_project -p 'examples/*' \
  -b frdmmcxn947@cm33_core0 -t armgcc -c debug --pick_one_target
```

注意：`list_project` 的 board 过滤采用 `board@core` 形式，而实际 `west build` 在该 SDK 仓库中采用 `-b board -Dcore_id=...`。这是当前版本工具接口的差异。

## 4. 推荐示例索引

| 能力 | 示例路径 |
|---|---|
| 串口 hello world | `examples/demo_apps/hello_world` |
| USB VCOM hello | `examples/demo_apps/hello_world_virtual_com` |
| RGB/板级 LED | `examples/demo_apps/led_blinky_peripheral` |
| FreeRTOS | `examples/freertos_examples/freertos_hello` |
| 触摸 | `examples/demo_apps/touch_sensing` |
| UART | `examples/driver_examples/lpuart/interrupt_transfer` |
| SPI | `examples/driver_examples/lpspi/interrupt` |
| I2C | `examples/driver_examples/lpi2c/interrupt` |
| I3C | `examples/driver_examples/i3c/polling_b2b_transfer/master` |
| PWM | `examples/driver_examples/pwm/pwm_3ph` |
| CAN FD | `examples/driver_examples/flexcan/canfd_interrupt_transfer` |
| Ethernet | `examples/driver_examples/mcx_enet/txrx_transfer_rxpoll` |
| lwIP ping/DHCP | `examples/lwip_examples/lwip_ping`、`lwip_dhcp` |
| USB host/device | `examples/usb_examples/...` |
| SmartDMA 摄像头 | `examples/display_examples/smartdma_camera_flexio_mculcd` |
| LVGL | `examples/lvgl_examples/lvgl_guider_bm` |
| TFLM 模型 | `examples/eiq_examples/tflm_modelrunner` |
| 摄像头 + CNN | `examples/eiq_examples/mpp/camera_mobilenet_view` |
| PowerQuad FFT | `examples/driver_examples/powerquad/benchmark_fft` |
| TF-M | `examples/tfm_examples/tfm_psatest_ns` |
| MCUboot OTA | `examples/ota_examples/ota_mcuboot_basic_psa` |
| 双核 RPMSG | `examples/multicore_examples/rpmsg_lite_pingpong` |

示例目录可能随 SDK 版本变化，执行 `west list_project` 是最可靠的发现方式。

## 5. 构建配置

### 一般应用

- `debug`：调试优化、内部 Flash 运行，适合开发。
- `release`：优化后构建。
- 当前板常见一般示例只有 `debug/release`。

### 外部 QSPI XIP

需要外部 Flash 启动/执行时，选择专门的 QSPI XIP 示例或配置。不要误以为普通 `debug` 构建就是外部 Flash 版本。

### Sysbuild

TrustZone、TF-M、OTA、双核等需要多个镜像的示例会带 `--sysbuild`：

```bash
west build -b frdmmcxn947 --sysbuild \
  mcuxsdk/examples/trustzone_examples/secure_faults_s \
  -Dcore_id=cm33_core0
```

多镜像工程的烧录顺序、链接地址和签名关系必须按示例 README 执行。

## 6. Kconfig 与 CMake

SDK 使用 Kconfig 选择组件、外设、板级配置和中间件。常用命令：

```bash
west build -t guiconfig
west build -t menuconfig
```

配置结果会写入构建目录的 `.config` 和 `mcux_config.h`。要长期保存项目配置：

- 把改动放在应用 `prj.conf` 或项目 overlay；
- 不直接修改生成的 build 目录；
- 不把 SDK 公共默认配置当作产品配置的唯一记录。

## 7. 工程组织建议

### 从示例开始

1. 选最接近目标功能的例子。
2. 先原样编译并上板验证。
3. 再创建独立 freestanding/in-repository 项目。
4. 把硬件初始化与业务逻辑分层。

### 推荐分层

```text
application/      产品流程、协议、状态机
services/         AI、网络、存储、升级、诊断
bsp_frdm/         板级外设封装
hal/              芯片/驱动抽象
config/           Kconfig、链接脚本、分区
platform/         RTOS、时钟、内存、日志
```

避免业务代码直接散落调用寄存器；寄存器访问应集中在驱动层，便于升级和测试。

## 8. 构建产物

典型输出在 `build/<project>/`：

| 文件 | 用途 |
|---|---|
| `*.elf` | 调试、符号、加载地址 |
| `*.bin` | Flash 编程工具 |
| `*.map` | 内存段、符号大小、链接分析 |
| `compile_commands.json` | IDE/静态分析 |
| `.config` | 最终 Kconfig |
| `mcux_config.h` | 生成配置头 |
| `runners.yaml` | west flash/debug 的板级 runner 信息 |

## 9. 版本锁和升级

```bash
cd "$MCXN_SDK"
west list > /path/to/sdk-revisions.txt
cd manifests
git rev-parse HEAD
```

项目 README 至少记录：

- SDK tag/revision；
- 所有 west 仓库 revision；
- 编译器版本；
- CMake/Ninja 版本；
- 构建命令；
- 板级配置文件；
- 已知 SDK 补丁。

升级 SDK 前在独立目录构建并运行板级回归，确认外设、AI、TLS、OTA 和双核行为没有变化。
