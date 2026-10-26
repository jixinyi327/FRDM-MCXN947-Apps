# FRDM-MCXN947 Apps

面向 FRDM-MCXN947 + LCD-PAR-S035 的多应用开发仓库。仓库包含可复现的开发环境、
MCUXpresso SDK 补丁、LVGL UI Skill，以及可复用的应用构建和烧写流程。

当前应用：

- `apps/touchscreen_tester/`：触摸屏测试应用；
- `apps/oscilloscope/`：ADC + DMA + PowerQuad FFT 双核示波器。

后续应用统一放在 `apps/<app_name>/`，各自拥有独立的 `README.md`、构建入口和资源配置。

触摸屏测试应用功能：

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
├── apps/oscilloscope/        双核 ADC 示波器与频谱分析
├── firmware/patches/         SDK 显示与触摸补丁
├── skills/lvgl-ui/           LVGL UI 设计、本地化和工程规范
├── skills/frdm-mcxn947-lvgl-ui-performance/
│                             本板显示性能与刷新约束
├── skills/frdm-mcxn947-adc-dma/
│                             ADC 分辨率、采样率与 eDMA 配置规范
├── development-plan/         环境、外设、硬件和排障文档
├── mcuxpresso-sdk/           本地 SDK，Git 忽略
├── toolchains/               本地 Arm GCC，Git 忽略
└── .venv/                    west/CMake/Ninja/Python 工具，Git 忽略
```

应用说明：

- [触摸屏测试](apps/touchscreen_tester/README.md)
- [示波器](apps/oscilloscope/README.md)

仓库约定：

- 目录和文件名使用 ASCII、小写描述性英文；中文保留在文档内容和界面文案中；
- 每个应用必须有独立构建入口，不与 SDK 或其他应用共享生成产物；
- 公共 UI、板级驱动和多应用共用代码通过明确模块复用，不从其他应用目录反向引用；
- 提交前运行 `bash development-plan/scripts/check_naming.sh` 检查路径命名。

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

## 一键环境管理（推荐）

克隆仓库后，直接运行纯 Bash 管理脚本：

```bash
cd <repository-directory>
bash development-plan/scripts/manage_dev_env.sh
```

交互菜单提供：

1. 检查 Linux/WSL2 环境和固定工具版本；
2. 在环境不满足时选择安装或修复；
3. 使用 SSH 或 HTTPS 检查 SDK Release；
4. 列出与当前 manifest 不一致或含本地改动的组件；
5. 确认后更新 SDK 和板级组件；
6. 创建独立的新版 SDK 工作区。

脚本支持 Ubuntu/Debian x86_64、WSL2 和 root 容器。当前固定下载的 NXP 工具是
Linux x86_64 版本，因此原生 Windows 必须使用 WSL2，ARM64 主机不在自动安装范围内。

常用非交互命令：

```bash
# 只检查，不修改文件
bash development-plan/scripts/manage_dev_env.sh --check

# SSH 检查更新
bash development-plan/scripts/manage_dev_env.sh \
  --check-updates --transport ssh --no-input

# SSH 更新当前 SDK；如果 SDK 有本地改动会拒绝执行
bash development-plan/scripts/manage_dev_env.sh \
  --update --transport ssh

# 不动当前 SDK，创建独立新版工作区
bash development-plan/scripts/manage_dev_env.sh \
  --fresh --transport ssh
```

SSH 首次配置：

```bash
ssh-keygen -t ed25519 -C "your-email@example.com"
cat ~/.ssh/id_ed25519.pub
```

把公钥添加到 GitHub 的 `Settings > SSH and GPG keys`，然后验证：

```bash
ssh -T git@github.com
```

脚本不会自动覆盖带有本地修改的 SDK。当前 `mcuxsdk/examples` 有显示补丁改动时，
应使用 `--fresh`，或先将改动保存为 `firmware/patches/` 中的补丁并清理工作树。

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
WORKSPACE="${MCXN_WORKSPACE:-$HOME/NXP/MCXN947}"
mkdir -p "$WORKSPACE"
cd "$WORKSPACE"
git clone <repository-url> .
```

如果目录中已经有项目文件，只执行：

```bash
cd "$WORKSPACE"
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

### 4. Node.js 与字体生成器

只有修改中文 UI 文案并重新生成字体时才需要这一组工具。固定版本可以避免不同设备
生成出不同字形数据：

```bash
mkdir -p downloads toolchains tools/lv_font_conv

curl -L --retry 5 \
  -o downloads/node-v24.22.1-linux-x64.tar.xz \
  'https://nodejs.org/dist/v24.22.1/node-v24.22.1-linux-x64.tar.xz'

printf '%s  %s\n' \
  '6c172a9ab6718f7b801fb5a7bd486af2ee5e32758086a04b84fcce2aa77095e6' \
  'downloads/node-v24.22.1-linux-x64.tar.xz' |
  sha256sum -c -

tar -xJf downloads/node-v24.22.1-linux-x64.tar.xz -C toolchains

source development-plan/scripts/env.sh
npm install \
  --prefix tools/lv_font_conv \
  --no-audit --no-fund --no-save \
  lv_font_conv@1.5.3
```

`manage_dev_env.sh --setup` 会自动完成本节。

### 5. Arm GNU Toolchain

```bash
mkdir -p toolchains downloads

curl -L --retry 5 \
  -o downloads/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi.tar.xz \
  'https://developer.arm.com/-/media/Files/downloads/gnu/14.2.rel1/binrel/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi.tar.xz'

tar -xJf \
  downloads/arm-gnu-toolchain-14.2.rel1-x86_64-arm-none-eabi.tar.xz \
  -C toolchains
```

### 6. MCUXpresso SDK 26.09

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
[WSL2 命令行环境](development-plan/05_wsl2_command_line_environment.md)。

### 7. LinkServer 26.9.130

```bash
cd "$WORKSPACE"
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

### 8. 设置当前终端环境

每次打开新终端都执行：

```bash
cd "$WORKSPACE"
source development-plan/scripts/env.sh
```

检查工具和板卡：

```bash
bash development-plan/scripts/check_env.sh
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
[WSL2 命令行环境](development-plan/05_wsl2_command_line_environment.md#5-udev-权限) 安装 udev 规则。

## 构建

推荐使用应用自带脚本：

```bash
cd "$WORKSPACE"
./apps/touchscreen_tester/build.sh
```

脚本会：

1. 从 `development-plan/scripts/env.sh` 加载固定工具链；
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
cd "$WORKSPACE"
source development-plan/scripts/env.sh
cd "$MCXN_SDK"

west build -b frdmmcxn947 \
  "$MCXN_ROOT/apps/touchscreen_tester" \
  -d build/touchscreen_tester_core0 -p always \
  --config debug -Dcore_id=cm33_core0
```

## 烧写

推荐：

```bash
cd "$WORKSPACE"
./apps/touchscreen_tester/build.sh --flash
```

该命令先完成全新构建，再通过 LinkServer 烧写当前连接的 FRDM-MCXN947。

等价的 west 命令：

```bash
cd "$WORKSPACE"
source development-plan/scripts/env.sh
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
bash development-plan/scripts/monitor.sh
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
- `$frdm-mcxn947-adc-dma`：按 12/16 位分辨率和 kS/s 请求规划稳定的 ADC、触发和
  eDMA ping-pong 采样路径。

调用示例：

```text
Use $lvgl-ui to review this LVGL screen and its localization.
Use $frdm-mcxn947-lvgl-ui-performance to diagnose display tearing.
Use $frdm-mcxn947-adc-dma with 16-bit resolution and a 2000k sample rate.
```

## 常见问题

### `west: unknown command "build"`

west 必须在 SDK 工作区内运行。先执行：

```bash
cd "$WORKSPACE/mcuxpresso-sdk"
source ../development-plan/scripts/env.sh
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

- [开发计划总览](development-plan/README.md)
- [首次烧录与串口验证](development-plan/07_first_flash_and_serial_validation.md)
- [SDK 工程与构建系统](development-plan/08_sdk_projects_and_build_system.md)
- [屏幕选型与适配](development-plan/display-integration/README.md)
- [触摸屏测试说明](apps/touchscreen_tester/README.md)
- [触摸屏性能记录](apps/touchscreen_tester/PERFORMANCE.md)
- [示波器说明](apps/oscilloscope/README.md)
- [示波器性能记录](apps/oscilloscope/PERFORMANCE.md)
