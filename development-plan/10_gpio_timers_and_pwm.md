# GPIO、定时器与 PWM

## 1. GPIO 基础

FRDM-MCXN947 的常用 GPIO：

| 功能 | 引脚 |
|---|---|
| 红灯 | P0_10 |
| 绿灯、SPI PCS/Arduino D10 | P0_27 |
| 蓝灯、Arduino D6 | P1_2 |
| 唤醒按键 | P0_23 |
| ISP 按键 | P0_6 |
| 触摸滑条 | P1_3 |

这些引脚不是“专用 LED/按键引脚”，还复用到 Arduino、mikroBUS、MCU-Link 等网络。业务配置前必须查第 20 篇。

### 配置检查

- 引脚功能号是否正确；
- 输入/输出方向；
- 上下拉策略；
- 驱动强度；
- 开漏/推挽；
- IO 电压域；
- 中断触发边沿/电平；
- 低功耗时是否保持。

### GPIO 中断

使用 PINT 时要注意：

- 中断标志与 GPIO 状态分离；
- 按键消抖不能只靠软件延时；
- 唤醒源与普通中断的配置不同；
- 多个引脚共享 PINT 时的源识别；
- ISR 内不做阻塞 I/O。

建议实验：`examples/driver_examples/gpio`、`pint/pattern_match`、`cmsis_driver_examples/gpio/button_toggle_led`。

## 2. 定时器资源

MCXN947 提供多种定时器：

- 5 × 32-bit CTimer；
- 2 × LPTimer；
- SCTimer/PWM；
- 24-bit MRT；
- UTICK；
- OS Event Timer；
- RTC；
- 2 × WWDT；
- 2 × Code Watchdog。

选择原则：

| 需求 | 推荐 |
|---|---|
| 通用周期中断/捕获/比较 | CTimer |
| 低功耗周期唤醒 | LPTimer/RTC |
| 多速率任务节拍 | MRT |
| 复杂状态 PWM/逻辑时序 | SCTimer |
| 任务超时监测 | WWDT/Code Watchdog |

## 3. PWM 与电机时序

MCXN947 有 2 个 FlexPWM 实例，每个 4 个子模块，总计可提供多路 PWM。关键能力：

- 互补输出；
- 死区；
- 边沿对齐/中心对齐；
- 故障输入快速关断；
- 分数边沿/抖动；
- 与 ADC、比较器、DMA 和事件网络同步。

开发时先做安全空载测试：

1. 只观察逻辑电平；
2. 设置最小占空比；
3. 检查死区；
4. 模拟故障输入；
5. 确认 PWM 在微秒级关断；
6. 最后才连接功率级。

不要在没有电流限制、隔离和急停的情况下连接电机。

## 4. DMA 与事件互联

MCXN947 有 2 个异步 DMA 模块，每模块 16 通道。适合：

- UART/SPI 批量搬运；
- ADC 连续采样；
- PWM/ADC 触发同步；
- LCD/摄像头数据流；
- 音频缓冲；
- 减少中断负载。

事件互联（XBAR/EVTG/PLU）可使比较器触发 PWM 关断、定时器触发 ADC、ADC 完成触发 DMA，从而降低响应延迟。

## 5. 实验步骤

### GPIO 实验

1. 配置 RGB 三色为输出。
2. 做呼吸/闪烁，验证每色独立。
3. 切换任意颜色对应的 Arduino 引脚用途，记录冲突。
4. 配置 SW2/SW3 为输入，测消抖。

### 定时器实验

1. CTimer 产生 1 ms/1 s 中断。
2. 用 SYSTICK 和 CTimer 比较抖动。
3. LPTimer 做低功耗唤醒。
4. WWDT 在任务卡死时复位并记录原因。

### PWM 实验

1. 单通道 1 kHz/20 kHz。
2. 互补输出和死区。
3. 故障输入强制关断。
4. ADC 触发与 PWM 同步。
5. 最后进入三相 FOC。

## 6. 验收标准

- GPIO 输出无毛刺，复位默认态安全；
- 按键无重复触发，唤醒后状态正确；
- 定时器长期运行无累计漂移；
- PWM 频率、占空比、死区满足示波器测量；
- 故障到 PWM 关断的延迟已测；
- DMA 无越界、无覆盖、无半包竞争；
- 看门狗复位后能区分冷启动与异常启动。
