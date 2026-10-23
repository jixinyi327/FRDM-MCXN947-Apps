# CAN FD 与 Ethernet

## 1. CAN FD

芯片提供 2 个 FlexCAN，支持 CAN 2.0B 和 CAN FD。FRDM 板默认连接 CAN0：

- TX：`P1_10`；
- RX：`P1_11`；
- 收发器：TJA1057GTK/3Z；
- 连接器：J10，CAN_H/CAN_L/5 V/GND。

### 软件步骤

1. 先跑 loopback，不依赖外部总线。
2. 再跑两个节点，使用短线和正确终端。
3. 配置仲裁段和数据段时间。
4. 验证标准帧、扩展帧、远程帧和 FD 长载荷。
5. 测试总线关闭、错误计数和恢复。
6. 测报文丢失、延迟和抖动。

### 物理层

- 双绞线；
- 总线两端约 120 Ω 终端；
- 节点短线；
- 所有节点共地策略；
- 共模范围、ESD、浪涌和隔离；
- 收发器供电和待机脚；
- 总线故障时的失效安全。

FRDM 板没有替代整车/工业现场的保护网络。自制板应按现场浪涌和隔离要求重新设计。

## 2. Ethernet 基础

板载路径：

```text
MCXN947 ENET0 -> RMII -> LAN8741A-EN -> 变压器/RJ45 -> 网络
```

能力：10/100 Mbps，支持 QoS。板上仅支持 RMII，50 MHz 有源晶振供 PHY，部分 TXD 被接地以固定 RMII 模式。

### 软件步骤

1. 裸机 PHY 读写。
2. PHY link 状态检查。
3. ENET 发送/接收回环或短包。
4. lwIP ping。
5. DHCP。
6. TCP/UDP 吞吐。
7. TLS/MQTT/HTTPS。
8. 断链、重连、低功耗与唤醒。

推荐示例：

- `examples/driver_examples/mcx_enet/txrx_transfer_rxpoll`
- `examples/lwip_examples/lwip_ping`
- `examples/lwip_examples/lwip_dhcp`
- `examples/lwip_examples/lwip_httpsrv`
- `examples/lwip_examples/lwip_mqtt`

## 3. Ethernet 硬件检查

- 50 MHz 时钟是否稳定、幅值是否正确；
- RMII TX/RX、TXEN、CRS_DV、REF_CLK 走线；
- PHY strap 配置是否正确；
- 变压器中心抽头和终端；
- RJ45 屏蔽地与系统地关系；
- PHY 复位时序；
- MDIO/MDC 上拉；
- 差分阻抗、长度匹配、参考平面；
- 与摄像头共享引脚是否被改焊破坏。

## 4. CAN 与 Ethernet 联合场景

工业网关常用组合：

- CAN 采集设备状态；
- MCU 做协议转换、过滤和缓存；
- Ethernet 上传 MQTT/HTTPS；
- TrustZone 隔离安全服务；
- 安全启动和签名升级。

在 FRDM 板上，CAN 和 Ethernet 默认可同时工作，但接摄像头时要查 P1_10/P1_11 和相关焊桥。

## 5. 网络性能与内存

MCXN947 是 10/100 Mbps 平台，512 KB SRAM 需要同时装：

- lwIP pbuf；
- TCP 窗口；
- TLS 会话；
- MQTT/TLS 证书；
- 应用队列；
- 双核共享区；
- OTA 下载缓冲。

性能测试必须同时看：

- 吞吐；
- CPU 占用；
- 最小剩余堆；
- 最大包延迟；
- TLS 握手时间；
- 断链恢复时间。

## 6. 推荐验收

### CAN FD

- 仲裁段和数据段速率符合设计；
- 标准/扩展帧和 FD 长帧通过；
- 终端开路/短路、错误注入可恢复；
- 24 小时持续通信无遗漏；
- 总线错误计数和恢复可观测。

### Ethernet

- 10/100 Mbps 自协商正常；
- ping 丢包率 0；
- DHCP 重连和地址变化可控；
- TCP 吞吐和延迟有基线；
- TLS 握手、证书过期、断网恢复均已测试；
- 摄像头改焊后的 Ethernet 场景有单独验证结果。

