# 首次烧录与串口验证

## 1. 目标

本实验只使用板载调试器和一个 Type-C 线，完成：

1. MCU-Link 正常枚举；
2. LinkServer 识别探针；
3. 烧录 SDK `hello_world`；
4. VCOM 输出和回显；
5. 复位后再次运行；
6. 调试断点可用。

## 2. 硬件准备

- FRDM-MCXN947；
- 一根数据完整的 USB-C 线；
- PC；
- 先用 J17（标有 MCU-Link 的 USB-C），不要先接 J11。

保持 J18、J19、J21 默认状态。

## 3. 上电检查

连接 J17 后：

- D5 绿色电源灯应亮；
- D10 `USB COMM` 绿色灯应在枚举后常亮；
- D11 红色状态灯应有心跳；
- D12 会在串口通信时闪。

若 D5 不亮：

1. 换线和 USB 口；
2. 确认接的是 J17；
3. 检查是否短接 J24；
4. 不要同时从多个入口接入 5 V。

若 D5 亮但 D10 不亮：

1. 检查 WSL USB 直通/Windows 驱动；
2. 检查设备管理器或 `lsusb`；
3. 若怀疑 MCU-Link 固件，按第 07 节恢复流程处理 J21。

## 4. 检查 LinkServer

```bash
source development-plan/scripts/env.sh
LinkServer probes
```

没有探针时，先确认 USB 是否直通；不要直接执行 Mass Erase。

确认支持目标：

```bash
LinkServer devices | grep FRDM-MCXN947
```

应看到类似：

```text
MCXN9XX  MCXN947  FRDM-MCXN947  cm33_core0, [cm33_core1]
```

方括号表示副核默认以 attach 方式调试。

## 5. 编译示例

```bash
bash development-plan/scripts/build_hello.sh
```

当前 SDK 仓库的正确路径是 `mcuxsdk/examples/demo_apps/hello_world`。不要把 core 写进 board 目录：

```bash
# 正确
west build -b frdmmcxn947 ... -Dcore_id=cm33_core0

# 该仓库布局下不要这样写
west build -b frdmmcxn947@cm33_core0 ...
```

## 6. 烧录

```bash
source development-plan/scripts/env.sh
cd "$MCXN_SDK"
west flash -d build/hello_world_core0 -r linkserver
```

如果有多块 NXP 板同时连接，要指定探针序号/序列号。使用 `LinkServer probes` 查看，后续在 runner 参数中传入探针选择；不要物理拔插不确定的设备。

## 7. 串口验证

最简单方式：

```bash
bash development-plan/scripts/monitor.sh
```

VS Code 也可使用 Serial Monitor：

- Port：MCU-Link VCOM；
- Baud：`115200`；
- Data：8；
- Parity：None；
- Stop：1。

按复位键 SW1，预期看到 `hello_world` 的启动信息和字符回显。具体文本以当前 SDK 示例为准。

若串口没有输出：

1. 确认 J18 开路；
2. 确认选择的是 MCU-Link VCOM，而不仅是 CMSIS-DAP；
3. 检查 `P1_8/P1_9` 是否被其他程序复用；
4. 检查 `/dev/ttyACM*` 权限；
5. 在 Windows 设备管理器中确认 COM 口。

## 8. 调试验证

```bash
west debug -d build/hello_world_core0 -r linkserver
```

验收：

- 能在 `main()` 下断点；
- 单步、继续、复位正常；
- 查看变量和寄存器正常；
- 断开调试后程序仍能运行；
- 再烧录一次不报探针占用。

若无法连接：

- 确认 J19 开路（使用板载 MCU-Link）；
- 确认目标没有进入不可调试生命周期；
- 确认没有第三方调试器同时占用；
- 不要立即 Mass Erase，先保存现象和日志。

## 9. 第一次烧录后的记录

建议记录：

| 项目 | 实际值 |
|---|---|
| 芯片丝印 | |
| MCU-Link 固件版本 | |
| LinkServer 版本 | |
| SDK tag/commit | |
| GCC 版本 | |
| VCOM 端口 | |
| 烧录配置 | |
| 复位/回显结果 | |
| 异常现象和日志 | |

## 10. 安全恢复

如果代码把调试口关闭或进入低功耗后无法连接：

1. 断电；
2. 按住 SW3/ISP；
3. 重新上电；
4. 尝试 UART ISP 或 LinkServer 的恢复连接；
5. 只在确认目标后执行 Mass Erase。

如果 MCU-Link 本身异常：

1. 断开 J17；
2. 短接 J21；
3. 重新连接 J17；
4. 用 NXP 工具恢复 LPC55S69 的 CMSIS-DAP 固件；
5. 断开，打开 J21，重新上电。

