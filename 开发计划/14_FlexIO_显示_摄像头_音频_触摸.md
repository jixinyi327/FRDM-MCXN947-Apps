# FlexIO、显示、摄像头、音频与触摸

LCD-PAR-S035 的完整专项分析、选型方法和适配流程见
[屏幕选型与适配](屏幕选型与适配/README.md)。

## 1. FlexIO

FlexIO 是可编程并行/串行接口，可模拟：

- LCD 总线；
- 摄像头控制时序；
- 额外 UART/SPI/I2C；
- PWM/波形；
- 自定义并串转换。

FRDM 的 J8 是 28-pin FlexIO/LCD 接口。板上明确要求显示模块只使用 3.3 V I/O；所有涉及的 Port 0/1/2/4 都必须配置为 3.3 V 电平。

## 2. LCD 显示

推荐扩展：

- `LCD-PAR-S035`：3.5 英寸、480×320、IPS、电容触摸；
- Mikroe TFT Proto 5 英寸电容屏。

J8 的信号包括：

- I2C SCL/SDA：P4_1/P4_0；
- 中断、背光、复位、D/C、CS、WR、RD、TE；
- 16-bit 数据 FXIO_D16 至 FXIO_D31。

关键共享：

- P4_5、P4_12、P4_13、P4_16、P4_20 等与 Arduino/电机采样网络共享；
- 使用 LCD 时应避免同时把那些 Arduino 网络当模拟/电机输入。

软件路线：

1. FlexIO GPIO 电平测试；
2. 8080/6800 总线写命令；
3. 单色块刷新；
4. DMA 整屏；
5. LVGL/emWin；
6. 触摸校准与 UI 性能。

## 3. 摄像头

J9 支持 32-pin 摄像头，NXP 测试过 OV7670。AN14191 给出的 SmartDMA 方案中：

- 数据 D0-D7：P1_4、P1_5、P1_6、P1_7、P3_4、P3_5、P1_10、P1_11；
- PCLK：P0_5；
- VSYNC：P0_4；
- HSYNC：P0_11；
- 摄像头时钟输出：P2_2；
- SCCB/I2C：P3_2/P3_3（FlexComm7）。

### 必须改焊

AN14191 和板级手册均指出，摄像头需要调整 `SJ16`、`SJ26`、`SJ27`。AN14191 描述为从 A 侧改到 B 侧。

默认改法的副作用：

- 会禁用或破坏 Ethernet；
- 也可能影响 CAN；
- 如果需要摄像头 + Ethernet，必须按板级手册给出的特定焊桥/飞线组合验证；
- 不要仅把模块插上就期待 Ethernet、CAN、摄像头同时工作。

### 内存预算

AN14191 的 480×320 单帧需要约 300 KB RAM，因此不能整帧双缓冲。当前方案用单个缓冲：

- SmartDMA 写当前帧；
- CPU/LCD 读上一帧；
- 利用显示刷新快于摄像头采集的时序；
- 以乒乓/半帧/分块方式优化。

任何提高分辨率或增加双缓冲的需求都要先核算 SRAM。

## 4. 摄像头 + NPU

推荐先跑：

- `examples/display_examples/smartdma_camera_flexio_mculcd`
- `examples/eiq_examples/mpp/camera_view`
- `examples/eiq_examples/mpp/camera_mobilenet_view`

验收：

- 帧同步正确；
- 无撕裂/倒置/色序错误；
- CPU 不因 LCD 刷新饿死；
- NPU 与 SmartDMA 同时运行稳定；
- 最小剩余 RAM 有安全余量。

## 5. 音频

芯片支持：

- SAI 2 路实例，最多 4 通道；
- PDM/MICFIL，最多 4 个数字麦克风；
- DMA 音频缓冲。

FRDM 不直接带 MEMS 麦克风/Codec，需要扩展。需注意：

- PDM 数据/时钟与 Pmod/摄像头/Arduino 共享；
- 3.3 V I/O；
- 音频时钟精度；
- DMA 半满/全满中断；
- 抗混叠/偏置/增益；
- 麦克风阵列的几何与相位一致性。

应用：

- 关键词识别；
- 异常声音检测；
- 语音命令；
- 工业声学监测。

## 6. 触摸

MCXN947 TSI：

- 最多 25 个自电容通道；
- 最多 8 TX × 17 RX；
- 支持防水自电容模式；
- 板载触摸滑条连接 P1_3。

开发时注意：

- P1_3 与 mikroBUS RST、MCU-Link SPI 桥共享；
- 触摸电极附近不要放高速/大电流走线；
- 接地参考和面板厚度改变会显著影响阈值；
- 必须做湿手、水膜、温漂、电源噪声和 EMC 测试；
- 自校准不能掩盖长期漂移。

推荐示例：`examples/demo_apps/touch_sensing`、`touch_cursor`、`touch_haptic`。

## 7. 实验顺序

1. FlexIO 输出固定电平。
2. LCD 单色/色块。
3. LCD DMA 刷新。
4. 触摸坐标。
5. 摄像头 I2C 识别。
6. 摄像头单帧抓取。
7. SmartDMA 连续采集。
8. 显示上一帧。
9. NPU 推理。
10. 综合 UI + 视觉 + 联网。

## 8. 验收标准

- LCD 在 3.3 V 下没有过冲和误写；
- 触摸坐标稳定且可通过湿手/温漂测试；
- 摄像头帧同步、色序和曝光正常；
- 改焊状态有明确标签和记录；
- 网络、CAN、摄像头组合能力有实测结论；
- 音频/视觉负载下最坏 RAM 和 CPU 占用留有裕量。
