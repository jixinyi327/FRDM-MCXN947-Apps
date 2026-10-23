# FRDM-MCXN947

资料、SDK、工具链和细分开发文档都在此目录。

- 开发计划入口：[开发计划/README.md](开发计划/README.md)
- SDK 工作区：`mcuxpresso-sdk/`
- 本地工具链：`toolchains/`
- 原始资料与补充参考手册：`docs/`
- 已编译验证的示例：`mcuxpresso-sdk/build/hello_world_core0/`
- LCD-PAR-S035 专题：[屏幕选型与适配](开发计划/屏幕选型与适配/README.md)
- UI 性能 Skill：[skills/frdm-mcxn947-lvgl-ui-performance](skills/frdm-mcxn947-lvgl-ui-performance/SKILL.md)
- 触摸屏测试工程：[apps/touchscreen_tester](apps/touchscreen_tester/README.md)

快速检查环境：

```bash
bash /home/jxy/NXP/MCXN947/开发计划/scripts/check_env.sh
```

首次连接开发板时：

1. 只把 USB-C 接到 J17（MCU-Link）。
2. 在 WSL 中完成 MCU-Link USB 直通，或改在 Windows 原生 VS Code 中调试。
3. 按 [首次烧录与串口验证](开发计划/07_首次烧录与串口验证.md) 操作。

当前命令行 SDK 已成功编译。FRDM-MCXN947 已通过 USB/IP 接入 WSL，MCU-Link
识别正常，`hello_world` 和 LVGL 屏幕示例均已成功烧录；LVGL 示例已运行并可通过
触摸拖动屏幕控件。

## UI 性能 Skill

项目内的 LVGL、显示、触摸和 UI 性能工作统一使用
`$frdm-mcxn947-lvgl-ui-performance`。它固定了已验证的双缓冲、单次 TE 同步、
SmartDMA 异步释放、60 FPS 任务节拍和双核归属边界，但不规定视觉风格。

直接调用示例：

```text
Use $frdm-mcxn947-lvgl-ui-performance to review this LVGL screen change.
```

独立校验和构建：

```bash
bash skills/frdm-mcxn947-lvgl-ui-performance/scripts/audit_ui_config.sh
bash skills/frdm-mcxn947-lvgl-ui-performance/scripts/build_ui.sh
```

## Git repository scope

The repository tracks project-owned documentation, scripts and configuration.
It intentionally ignores the 7.8 GB SDK workspace, toolchains, `.venv`, large
vendor documents and all build outputs. `VERSIONS.md` records the exact software
and source revisions required to recreate them.
