# 双核 UI 架构规划

## 1. 60 FPS 目标

当前 LVGL 主循环周期为：

```c
#define LVGL_TASK_PERIOD_TICK 16U
```

SysTick 为 1 ms，因此名义上限为：

```text
1000 / 16 = 62.5 FPS
```

实际帧率还会受到以下限制：

- ST7796 面板自身约 60 Hz 扫描；
- TE 等待；
- FlexIO 16 位 8080 写入带宽；
- SmartDMA 启动和完成开销；
- 拖动控件造成的无效区域大小；
- LVGL 字体、透明度、阴影和动画复杂度；
- 另一核心访问 Flash/SRAM/外设造成的总线竞争。

因此应称为“约 60 FPS 上限”，不能保证每个快速拖动帧都达到 60 FPS。

## 2. 当前单核基线

`lvgl_guider_bm` 目前全部运行在 `cm33_core0`：

- UI 和 LVGL；
- LCD-PAR-S035/ST7796S；
- GT911 触摸；
- SmartDMA；
- 串口日志；
- 业务逻辑。

当前构建约占用：

```text
m_text: 313 KB / 767 KB
m_data: 211 KB / 312 KB
```

UI 静态 RAM 已占 `m_data` 的约 66%，后续功能不应继续全部堆在 core0。

## 3. 推荐职责划分

### Core0：UI/交互核

- 系统启动和电源管理；
- LVGL、GUI Guider 和 UI 状态机；
- ST7796S 显示驱动；
- GT911 触摸和手势；
- 背光与时序；
- LCD 动画和页面切换；
- UI 与功能核通信接口。

只有 core0 可以访问：

- FlexIO 和 LCD 数据/控制引脚；
- TE/RST/CS/D/C/WR/RD；
- 背光 GPIO；
- GT911 I2C 和 INT；
- LVGL draw buffer；
- `st7796s_handle_t` 和显示初始化状态。

### Core1：功能/实时核

根据产品选择：

- 电机、电源和 PWM 控制；
- ADC/SINC/编码器；
- CAN/Ethernet 实时收发；
- 传感器采样；
- 通信协议栈；
- AI 推理调度；
- 安全服务；
- 文件系统与数据记录。

Core1 不直接写 LCD，core0 也不直接操作 core1 的外设寄存器。

## 4. 核间通信

### 状态快照

适合 UI 周期性显示：

- 温度、电压、电流；
- 速度、位置；
- 设备状态和告警；
- 网络连接状态。

更新频率通常为 10–50 Hz，不需要每个 UI 帧都通信。

推荐结构：

```text
Core1 写共享状态
-> 写序号/版本
-> Core0 读取
-> 再次检查序号，防止读到半更新数据
```

### 命令与事件

推荐使用：

- Mailbox；
- RPMsg-Lite；
- eRPC；
- SEMA42 保护短临界区。

RPMsg 适合：

- UI 按钮发送命令；
- core1 告警触发弹窗；
- 模式切换；
- 升级和诊断请求。

### 大块数据

- 通过共享内存环或文件/Flash；
- 控制消息只传地址、长度和版本；
- 明确数据所有权和释放方；
- 不通过 RPMsg 直接复制整帧图片。

## 5. 共享内存规则

- 固定宽度类型；
- 协议版本和魔数；
- 序号或双缓冲；
- 长度、地址和边界检查；
- 内存屏障；
- 一侧异常时的超时和重建；
- 复位后重新握手；
- 不共享裸指针所有权。

不允许两个核同时操作 LCD framebuffer、ST7796S 命令或触摸 I2C。

## 6. 多镜像工程

推荐从 SDK 示例起步：

- `examples/multicore_examples/multicore_manager/primary`
- `examples/multicore_examples/rpmsg_lite_pingpong`
- `examples/driver_examples/sema42/dual_core/primary`

多镜像工程通常使用 `--sysbuild`：

```bash
source development-plan/scripts/env.sh
cd "$MCXN_SDK"

west build -b frdmmcxn947 --sysbuild \
  mcuxsdk/examples/multicore_examples/rpmsg_lite_pingpong \
  -Dcore_id=cm33_core0
```

具体路径以当前 SDK 的 `west list_project` 为准。

## 7. 推荐启动关系

```text
Core0 primary
├── 板级和时钟初始化
├── 启动 Core1
├── LCD/GT911/LVGL
├── UI 任务
└── IPC 服务

Core1 secondary
├── 功能初始化
├── 电机/传感器/网络
├── 实时控制循环
└── IPC 服务
```

谁负责启动 core1 由 sysbuild/板级启动流程决定，不能两个镜像各自重复初始化共享时钟和外设。

## 8. 资源规划

### RAM

- core0：LVGL heap、两块 1/4 屏缓冲、触摸和 UI；
- core1：控制、网络、AI 或文件系统；
- 共享：RPMsg 队列和状态快照；
- 每核独立栈、堆和异常余量。

当前 UI 静态 RAM 约 211 KB。core0 不应再承担大型 AI 张量或大网络缓存。

### Flash

- UI 字体/图片可放内部 Flash 或外部 QSPI；
- core1 代码/常量分区；
- Bootloader、安全升级和双镜像布局需要统一；
- 外部 Flash 访问会与两核代码执行竞争。

### CPU 和总线

从 30 FPS 提到约 60 FPS 会使 UI 核心的绘制和写屏负载接近翻倍：

- CPU 绘制增加；
- FlexIO/SmartDMA 使用增加；
- SRAM 带宽增加；
- 触摸处理和 UI 状态更新增加。

功能迁移到 core1 后，core0 可以更专注 UI，但两核同时访问 Flash/SRAM 仍有竞争，必须实测。

## 9. 推荐实施顺序

1. 先在 core0 单核确认 60 FPS 上限和拖动效果。
2. 测量实际 FPS、最坏帧时间和 RAM 余量。
3. 创建最小 core1 启动镜像并验证调试。
4. 加入 RPMsg ping-pong。
5. 迁移一个功能，例如 ADC 或 CAN。
6. 定义共享状态协议。
7. 最后迁移网络/AI 等大模块。

不要同时重构 UI、RTOS、双核、安全启动和存储。

## 10. 调试

LinkServer 可开放两个核心：

```bash
LinkServer gdbserver MCXN947:FRDM-MCXN947 -c all
```

检查：

- 两核启动顺序；
- 共享内存地址是否一致；
- 主核复位后副核状态；
- 一侧卡死时另一侧是否可恢复；
- UI 卡顿是否由总线竞争或 IPC 引起；
- 调试断点是否破坏实时控制。

## 11. 验收

- UI 名义 60 FPS，并记录实测平均/最坏 FPS；
- core1 功能在 UI 满负载下仍满足截止时间；
- 快速拖动不会卡住功能核；
- 共享数据无撕裂、越界或旧值覆盖；
- 一侧复位后系统可恢复；
- 两核 RAM/Flash 余量可量化；
- 发布版本不依赖调试器暂停。
