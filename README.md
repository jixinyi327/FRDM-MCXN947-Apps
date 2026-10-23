# FRDM-MCXN947 Touchscreen Tester

FRDM-MCXN947 + LCD-PAR-S035 触摸屏测试工程。项目包含可复现的开发环境说明、
MCUXpresso SDK 补丁、LVGL UI Skill，以及运行在 `cm33_core0` 上的触摸屏测试应用。

当前应用功能：

- 自由绘制；
- 触摸坏块检测；
- 触摸点击精度测试；
- 设置页；
- 中英文切换，默认中文；
- 破坏性 Reset 操作确认；
- 双行状态输出；
- 双缓冲、SmartDMA 和 VBlank TE 同步。

## Repository status

本目录已经是 Git 仓库，当前分支为 `main`。不需要再次执行 `git init`。

```bash
git status
git log --oneline
```

如需发布到远端：

```bash
git remote add origin <repository-url>
git push -u origin main
```

`mcuxpresso-sdk/`、`toolchains/`、`.venv/`、`docs/`、`LCD-PAR-S035/`、工具安装包
和所有构建产物已被 `.gitignore` 排除。仓库只跟踪项目源代码、补丁、脚本和文档。

精确版本记录见 [VERSIONS.md](VERSIONS.md)。

## 目录结构

```text
.
├── apps/touchscreen_tester/  触摸屏测试应用
├── firmware/patches/         SDK 显示与触摸补丁
├── skills/lvgl-ui/           LVGL UI 设计、本地化和工程规范
├── skills/frdm-mcxn947-lvgl-ui-performance/
│                             本板显示性能与刷新约束
├── 开发计划/                 环境、外设、硬件和排障文档
├── mcuxpresso-sdk/           本地 SDK，Git 忽略
├── toolchains/               本地 Arm GCC，Git 忽略
└── .venv/                    west/CMake/Ninja/Python 工具，Git 忽略
```

应用说明：[apps/touchscreen_tester/README.md](apps/touchscreen_tester/README.md)

## 硬件准备

- FRDM-MCXN947 开发板；
- LCD-PAR-S035 屏幕模块；
- 一根支持数据传输的 USB-C 线；
- 主机运行 WSL2 Ubuntu 24.04，或使用 Windows 原生 MCUXpresso/VS Code。

把 USB-C 接到板上的 **J17（MCU-Link）**。正常情况下：

- D5 电源灯亮；
- D10 USB COMM 灯亮；
- LinkServer 能识别 `MCXN947:FRDM-MCXN947`。

屏幕接到板卡的 LCD 接口。不要同时从多个入口给板卡供电。

## 开发环境搭建

以下步骤用于新机器复现。当前工作区已安装时，可以直接跳到“每次打开终端”。

### 1. 系统依赖

```bash
sudo apt update
sudo apt install -y \
  git python3 python3-venv curl xz-utils unzip \
  build-essential ninja-build pkg-config usbutils
```

### 2. 创建工作区

```bash
mkdir -p /home/jxy/NXP/MCXN947
cd /home/jxy/NXP/MCXN947
git clone <repository-url> .
```

如果目录中已经有项目文件，只执行：

```bash
cd /home/jxy/NXP/MCXN947
```

### 3. Python 工具和 west

```bash
python3 -m venv .venv
.venv/bin/python -m pip install --upgrade pip

.venv/bin/pip install \
  west==1.5.0 \
  cmake==3.31.10 \
  ninja==1.13.2 \
  pyserial==3.5 \
  pyelftools==0.33 \
  jsonschema==4.26.0 \
  requests==2.34.2 \
  Jinja2==3.1.6
```

### 4. Arm GNU Toolchain

```bash
mkdir -p toolchains downloads

curl -L --retry 5 \
  -o downloads/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi.tar.xz \
  'https://developer.arm.com/-/media/Files/downloads/gnu/14.2.rel1/binrel/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi.tar.xz'

tar -xJf \
  downloads/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi.tar.xz \
  -C toolchains
```

### 5. MCUXpresso SDK 26.09

```bash
export PATH="$PWD/.venv/bin:$PATH"

west init \
  -m https://github.com/nxp-mcuxpresso/mcuxsdk-manifests.git \
  "$PWD/mcuxpresso-sdk" \
  --mr v26.09.00

cd mcuxpresso-sdk
west update_board --set board frdmmcxn947
```

SDK 完整同步约 7.8 GB，下载时间取决于网络。详细流程见
[WSL2 命令行环境](开发计划/05_WSL2命令行环境.md)。

### 6. LinkServer 26.9.130

```bash
cd /home/jxy/NXP/MCXN947
mkdir -p downloads tools/linkserver

curl -L --retry 3 \
  -o downloads/LinkServer_26.9.130.x86_64.deb.bin \
  'https://www.nxp.com/lgfiles/updates/mcuxpresso/LinkServer_26.9.130.x86_64.deb.bin'

chmod +x downloads/LinkServer_26.9.130.x86_64.deb.bin

env DISPLAY= downloads/LinkServer_26.9.130.x86_64.deb.bin \
  --noexec --target downloads/linkserver_extract

dpkg-deb -x \
  downloads/linkserver_extract/LinkServer_26.9.130.x86_64.deb \
  tools/linkserver
```

### 7. 设置当前终端环境

每次打开新终端都执行：

```bash
cd /home/jxy/NXP/MCXN947
source 开发计划/scripts/env.sh
```

检查工具和板卡：

```bash
bash 开发计划/scripts/check_env.sh
```

## WSL2 USB 直通

Windows 管理员 PowerShell：

```powershell
winget install --interactive --exact dorssel.usbipd-win
usbipd list
usbipd bind --busid <BUSID>
usbipd attach --wsl --busid <BUSID>
```

WSL 中验证：

```bash
lsusb
LinkServer probes
ls -l /dev/ttyACM* /dev/ttyUSB*
```

重新插拔开发板或重启 WSL 后，通常需要重新执行 `usbipd attach`。

如调试器权限不足，按
[WSL2 命令行环境](开发计划/05_WSL2命令行环境.md#5-udev-权限) 安装 udev 规则。

## 构建

推荐使用应用自带脚本：

```bash
cd /home/jxy/NXP/MCXN947
./apps/touchscreen_tester/build.sh
```

脚本会：

1. 从 `开发计划/scripts/env.sh` 加载固定工具链；
2. 用 west 为 `frdmmcxn947`、`cm33_core0` 构建应用；
3. 输出 ELF/BIN；
4. 自动执行 UI 配置审计。

构建产物：

```text
mcuxpresso-sdk/build/touchscreen_tester_core0/touchscreen_tester_cm33_core0.elf
mcuxpresso-sdk/build/touchscreen_tester_core0/touchscreen_tester_cm33_core0.bin
```

等价的 west 命令：

```bash
cd /home/jxy/NXP/MCXN947
source 开发计划/scripts/env.sh
cd "$MCXN_SDK"

west build -b frdmmcxn947 \
  /home/jxy/NXP/MCXN947/apps/touchscreen_tester \
  -d build/touchscreen_tester_core0 -p always \
  --config debug -Dcore_id=cm33_core0
```

## 烧写

推荐：

```bash
cd /home/jxy/NXP/MCXN947
./apps/touchscreen_tester/build.sh --flash
```

该命令先完成全新构建，再通过 LinkServer 烧写当前连接的 FRDM-MCXN947。

等价的 west 命令：

```bash
cd /home/jxy/NXP/MCXN947
source 开发计划/scripts/env.sh
cd "$MCXN_SDK"
west flash -d build/touchscreen_tester_core0 -r linkserver
```

如果 west flash 遇到探针或外部 Flash 探测的瞬时失败，可先线复位再重试。也可使用
LinkServer 直接烧写 BIN：

```bash
LinkServer probes

LinkServer flash \
  -p <probe-serial-or-index> \
  -u none \
  'MCXN947:FRDM-MCXN947' \
  load build/touchscreen_tester_core0/touchscreen_tester_cm33_core0.bin \
  --addr 0x0
```

多块 NXP 板同时连接时必须显式指定探针。不要对不确定的目标执行 Mass Erase。

## 串口验证

串口参数：

```text
115200-8-N-1
```

运行：

```bash
bash 开发计划/scripts/monitor.sh
```

复位后应看到：

```text
Touchscreen tester started: 480x320, 16 ms UI cadence
```

如果没有串口输出，检查 J18、MCU-Link VCOM、`/dev/ttyACM*` 权限和 USB 直通。

## UI 审计

```bash
LVGL_UI_INPUT=touch \
  bash skills/lvgl-ui/scripts/audit_lvgl_ui.sh apps/touchscreen_tester/ui
```

该命令检查样式集中、控件状态、触控命中、中英文字体配置等。

显示性能审计由构建脚本自动运行，也可手动执行：

```bash
UI_APP=touchscreen_tester \
UI_SOURCE="$PWD/apps/touchscreen_tester" \
UI_MAIN="$PWD/apps/touchscreen_tester/main.c" \
UI_ELF="$PWD/mcuxpresso-sdk/build/touchscreen_tester_core0/touchscreen_tester_cm33_core0.elf" \
bash skills/frdm-mcxn947-lvgl-ui-performance/scripts/audit_ui_config.sh
```

## 开发 Skill

- `$lvgl-ui`：信息架构、设计令牌、控件状态、中英文、字体、焦点和工程规范；
- `$frdm-mcxn947-lvgl-ui-performance`：双缓冲、TE、SmartDMA、撕裂与双核刷新边界。

调用示例：

```text
Use $lvgl-ui to review this LVGL screen and its localization.
Use $frdm-mcxn947-lvgl-ui-performance to diagnose display tearing.
```

## 常见问题

### `west: unknown command "build"`

west 必须在 SDK 工作区内运行。先执行：

```bash
cd /home/jxy/NXP/MCXN947/mcuxpresso-sdk
source ../开发计划/scripts/env.sh
```

使用应用脚本时可直接从仓库根目录运行，不需要手动进入 SDK。

### 看不到 MCU-Link

检查 J17、USB 数据线、Windows `usbipd` 直通和 `lsusb`。确认后运行：

```bash
LinkServer probes
```

### 中文显示方框或空白

必须同时配置：

```conf
CONFIG_LV_TXT_ENC_UTF8=y
CONFIG_LV_TXT_ENC_ASCII=n
CONFIG_LV_USE_FONT_COMPRESSED=y
```

修改中文文案后重新生成字体：

```bash
./apps/touchscreen_tester/scripts/generate_fonts.sh
```

### LinkServer 烧写偶发失败

先执行：

```bash
LinkServer reset 'MCXN947:FRDM-MCXN947' -p <probe> --ms 250 --update-mode none
```

然后重新烧写。失败发生在加载镜像之前时，不要立即执行 Mass Erase。

## 相关文档

- [开发计划总览](开发计划/README.md)
- [首次烧录与串口验证](开发计划/07_首次烧录与串口验证.md)
- [SDK 工程与构建系统](开发计划/08_SDK工程与构建系统.md)
- [屏幕选型与适配](开发计划/屏幕选型与适配/README.md)
- [应用详细说明](apps/touchscreen_tester/README.md)
- [性能记录](apps/touchscreen_tester/PERFORMANCE.md)
