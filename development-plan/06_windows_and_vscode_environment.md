# Windows 与 VS Code 环境

## 1. 选择哪种开发方式

这块板有三种常用路径：

| 路径 | 优点 | 缺点 | 推荐场景 |
|---|---|---|---|
| Windows 原生 VS Code + MCUXpresso | USB 直接可用、配置简单 | 工具链和工程在 Windows | 快速上手、长期调试 |
| WSL2 命令行 + VS Code Remote | Linux 构建可脚本化、与 CI 接近 | 需 usbipd USB 直通、udev 权限 | 自动化构建、Linux 工程 |
| MCUXpresso IDE 独立版 | NXP 生态完整、调试配置成熟 | 较重、项目格式偏传统 | 习惯 NXP IDE 或需要图形配置 |

当前 WSL 已装好命令行工具链，因此建议把 WSL 作为构建/版本管理环境；日常调试可优先使用 Windows 原生 VS Code，避免 USB 直通问题。

## 2. Windows 原生 VS Code

### 2.1 安装软件

1. 安装 VS Code Windows 版。
2. 安装 NXP 发布者 `NXPSemiconductors` 的 **MCUXpresso for VS Code** 扩展。
3. 从扩展欢迎页打开 **MCUXpresso Installer**。
4. 选择 `MCUXpresso SDK Developer` 或按需安装：
   - Arm GNU Toolchain 14.2.x；
   - CMake/Ninja/Git；
   - LinkServer；
   - MCU-Link 固件工具。

已经存在本地 GCC 时，也可以在扩展的工具链设置中注册该目录。对 Windows，目录根应包含 `bin/arm-none-eabi-gcc.exe`。

### 2.2 导入 SDK

两种方式：

- 在扩展中重新创建一个由 NXP 管理的 SDK；
- 把 WSL 内的 SDK 复制到 Windows 可访问目录后导入。

跨 WSL/Windows 访问 `\\wsl$\...` 路径可能降低文件系统性能，也会让工具路径和权限更复杂。长期项目建议在 Windows 盘放一份 SDK，或在 WSL 中完成完整构建，只把调试交给 Windows。

### 2.3 导入示例

在 Quickstart Panel 选择 **Import Example from Repository**：

1. SDK：选择已导入的 MCUXpresso SDK。
2. Toolchain：选择 Arm GNU Toolchain。
3. Board：`FRDM-MCXN947`。
4. Template：例如 `demo_apps`。
5. Application：先选 `hello_world`。
6. Application type：
   - Freestanding：独立目录，适合产品项目；
   - In Repository：直接使用 SDK 内示例。
7. Import。

### 2.4 构建、串口和调试

- PROJECTS 视图中点 Build Project。
- 打开 Serial Monitor，波特率 115200。
- 使用 MCU-Link 调试配置启动 Debug。
- 复位后确认串口输出。

## 3. WSL 中的 VS Code

当前 WSL Server 中已安装：

```text
nxpsemiconductors.mcuxpresso@26.9.44
ms-vscode.cmake-tools@1.24.42
ms-vscode.vscode-serial-monitor@0.13.1
```

打开目录：

```bash
cd <repository-directory>
code .
```

`.vscode/settings.json` 使用工作区相对路径设置 GCC 和 LinkServer。若扩展把
`mcuxpresso.linkserver.path` 当作 machine-scope 设置而不读取 workspace 值，可在
WSL 的 User Settings JSON 中使用当前机器的实际路径：

```json
{
  "mcuxpresso.linkserver.path": "<absolute-path-to-repository>/tools/linkserver/usr/local/LinkServer_26.9.130/LinkServer"
}
```

然后重新加载 VS Code 窗口。

### 3.1 WSL 调用 Windows PowerShell

```bash
powershell.exe -NoProfile -Command 'usbipd list'
```

若命令提示找不到 usbipd，先在 Windows 用管理员 PowerShell 安装：

```powershell
winget install --interactive --exact dorssel.usbipd-win
```

### 3.2 USB 直通

```powershell
usbipd list
usbipd bind --busid <BUSID>
usbipd attach --wsl --busid <BUSID>
```

WSL 侧：

```bash
lsusb
LinkServer probes
ls -l /dev/ttyACM* /dev/ttyUSB*
```

若 MCU-Link 是复合设备，可能需要同时关注 CMSIS-DAP 和 VCOM 两个接口。串口不出现而探针可见时，优先检查 J18、Windows 驱动和 usbipd 绑定的是否是目标 MCU 的调试器。

## 4. MCUXpresso Installer 的作用

Installer 可以统一安装：

- Arm GNU Toolchain；
- CMake、Ninja、Git；
- LinkServer；
- SEGGER J-Link / PEmicro 驱动；
- MCU-Link 固件更新工具；
- 某些 SDK/IDE 辅助组件。

当前源码构建并不依赖完整 IDE，但使用 VS Code 扩展导入示例、自动生成配置和调试配置时，Installer 能减少路径与版本不匹配。

## 5. 工程推荐组合

### 组合 A：最快上手

- Windows VS Code；
- MCUXpresso 扩展；
- NXP Installer 安装 GCC + LinkServer；
- 直接在 Windows 原生终端构建和调试。

### 组合 B：Linux 构建 + Windows 调试

- WSL 中执行 west/CMake/GCC 构建；
- Windows 中打开同一源码副本或通过独立格式化产物调试；
- 需要自行保证源码同步和构建产物路径一致。

### 组合 C：纯 WSL

- WSL 构建；
- usbipd 直通 MCU-Link；
- LinkServer + GDB/VS Code 扩展；
- 需要 udev 规则和稳定的 USB 重连脚本。

建议团队 CI 使用组合 B/C 的构建命令，同时至少保留一台 Windows 机器做 USB/调试交叉验证。

## 6. 首次在扩展中新建产品工程

1. 先导入 `hello_world` 验证工具链。
2. 再选择与产品最接近的示例。
3. 使用 **Freestanding** 导出到独立目录。
4. 立刻把工程提交到版本库。
5. 固定 SDK 版本和工具链版本。
6. 在 README 中写清构建、烧录、串口和恢复命令。

不要直接在共用的 SDK 示例目录里长期开发，否则 SDK 更新时容易丢失或产生冲突。

