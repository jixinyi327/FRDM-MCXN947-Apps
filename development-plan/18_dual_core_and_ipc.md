# 双核开发与核间通信

## 1. 双核角色划分

MCXN947 有两个 150 MHz Cortex-M33。常见划分：

| 主核 `cm33_core0` | 副核 `cm33_core1` |
|---|---|
| 启动、系统管理 | 实时控制 |
| 网络/文件系统 | 传感器采集 |
| 安全服务 | 电机/音频实时环 |
| UI/业务状态机 | 专用协议处理 |

划分原则：

- 实时性和安全隔离驱动分工；
- 避免两边同时写同一外设；
- 明确共享内存所有权；
- 不在核间通信上放硬实时关键路径；
- CPU 负载留有余量。

## 2. 当前构建识别

```bash
source development-plan/scripts/env.sh
cd "$MCXN_SDK"
west boards -b frdmmcxn947
```

应看到：

```text
cm33_core0, cm33_core1
```

一般应用构建主核：

```bash
west build -b frdmmcxn947 \
  mcuxsdk/examples/demo_apps/hello_world \
  -Dcore_id=cm33_core0
```

副核：

```bash
west build -b frdmmcxn947 \
  mcuxsdk/examples/demo_apps/hello_world \
  -Dcore_id=cm33_core1
```

但真正的双核产品应从双核示例开始，而不是分别手工烧录两个完全独立应用。

## 3. 推荐示例

| 目标 | 示例 |
|---|---|
| 生命周期管理 | `examples/multicore_examples/multicore_manager/primary` |
| RPMSG ping-pong | `examples/multicore_examples/rpmsg_lite_pingpong` |
| 安全/非安全双核通信 | `examples/multicore_examples/rpmsg_lite_pingpong_tzm/primary_ns` |
| 硬件信号量 | `examples/driver_examples/sema42/dual_core/primary` |
| eRPC | SDK 的 `erpc` 示例/中间件 |

这些工程通常需要 `--sysbuild`：

```bash
west build -b frdmmcxn947 --sysbuild \
  mcuxsdk/examples/multicore_examples/multicore_manager/primary \
  -Dcore_id=cm33_core0
```

## 4. 核间通信机制

### Mailbox

适合短消息、事件通知、门铃式通信。注意中断清除、消息覆盖和优先级。

### SEMA42

硬件信号量适合保护共享资源。只在短临界区持有，必须有超时和异常恢复。

### RPMsg-Lite

适合带消息头的异步通信，便于多通道和虚拟队列。需要：

- 共享内存地址一致；
- 双方启动顺序；
- 回调不阻塞；
- 消息大小和队列深度；
- 超时和重同步。

### eRPC

适合把副核功能封装成远程过程调用，但要评估序列化开销和错误处理。

## 5. 共享内存设计

建议定义单一共享接口头：

```c
typedef struct {
    volatile uint32_t sequence;
    uint32_t payload_length;
    uint8_t  payload[MAX_PAYLOAD];
    uint32_t checksum;
} shared_packet_t;
```

必须明确：

- 哪个核只写、哪个核只读；
- 双缓冲还是环缓冲；
- 内存屏障/编译器屏障；
- 对齐和缓存属性；
- 写入完成通知顺序；
- 复位时如何清理；
- 一侧异常时如何超时恢复。

## 6. 外设所有权

建立一张外设归属表：

| 外设 | 拥有核 | 另一核访问方式 |
|---|---|---|
| Ethernet | core0 | RPC/RPMsg |
| CAN0 | core0 | 共享消息队列 |
| ADC/电机 PWM | core1 | 共享状态快照 |
| Flash 写入 | core0 | 服务 API |
| 安全密钥 | 安全服务 | 受控命令 |

禁止两边直接配置同一外设寄存器。

## 7. 调试

- LinkServer 默认主核调试，副核以 attach 方式开放独立端口；
- 多核断点可能改变实时时序；
- 核间死锁要能通过 trace/日志定位；
- 两侧都应有独立看门狗/心跳；
- 复位策略需明确是整芯片、主核还是副核；
- 不要在一个核暂停时依赖另一个核完成通信。

## 8. 性能和功耗

双核不一定更快：

- 通信和同步有开销；
- 两核同时访问 Flash/SRAM 会争用；
- 睡眠/唤醒需协调；
- 调试和日志会干扰。

先测单核基线，再测双核拆分后的端到端延迟、吞吐、功耗和抖动。

## 9. 验收

- 两核都能独立复位并恢复；
- 共享内存无撕裂/越界；
- 通信在满载和压力下不丢消息；
- 一侧卡死时另一侧可检测并恢复；
- 外设所有权无冲突；
- 烧录、调试、掉电重启流程可重复；
- sysbuild 镜像版本和签名一致。
