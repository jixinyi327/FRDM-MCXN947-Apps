# Touchscreen Tester

FRDM-MCXN947 + LCD-PAR-S035 的触摸屏测试工程。应用位于主 Git 仓库，
显示与触摸驱动继续使用 `firmware/patches/` 中已验证、可复现的 SDK 改动。

## 页面

| 页面 | 功能 |
|---|---|
| Free Draw | 400x160 单色画布，连续绘制并检查轨迹连续性 |
| Block Check | 12x7 检测网格，填充每个可达区域并统计覆盖率 |
| Touch Accuracy | 移动靶点，统计最近、平均、最佳误差和测试次数 |
| Settings | 绘制线宽、触摸坐标显示、测试数据重置 |

右下角圆形 `TAB` 按钮打开纵向标签列表。选择标签后，菜单向按钮方向收回，
页面按索引方向滑入：进入后面的页面时从右侧进入，返回前面的页面时从左侧进入。
点击菜单外部会原路收回。

## 交互和性能约束

- 所有页面只由 `cm33_core0` 创建和绘制；
- LCD/GT911 只由一个核访问；
- LVGL 使用两块 480x80 RGB565 局部缓冲；
- VBlank TE 每次完整 LVGL 更新只等待一次；
- SmartDMA 完成后释放当前 LVGL 缓冲；
- UI 处理节拍固定为 16 ms；
- 页面切换只平移 14 px，避免大范围透明合成；
- 自由绘制只失效当前线段覆盖的矩形，不刷新整块画布。

UI 视觉元素目前使用基础主题。颜色、字体和组件风格应由后续独立 UI 风格 Skill
统一约束，性能行为继续由 `$frdm-mcxn947-lvgl-ui-performance` 约束。

## 构建

```bash
./apps/touchscreen_tester/build.sh
```

烧录到当前连接的 FRDM-MCXN947：

```bash
./apps/touchscreen_tester/build.sh --flash
```

构建后会运行 UI 配置审计。当前预期静态 RAM 约 276 KiB，Flash 约 383 KiB。

## 文件结构

```text
apps/touchscreen_tester/
├── CMakeLists.txt
├── build.sh
├── main.c
├── prj.conf
├── PERFORMANCE.md
└── ui/
    ├── app_ui.c                页面切换、菜单和动画
    ├── screen_free_draw.c     自由绘制
    ├── screen_block_check.c   坏块检测
    ├── screen_accuracy.c      点击精度
    ├── screen_settings.c      设置
    └── ui_common.c            共享布局与样式入口
```

## 实机验收

1. 静止页面保持 60 秒，无闪烁、花屏或复位；
2. 慢速拖动蓝色/黑色轨迹，线条连续且无明显撕裂；
3. 快速拖动，允许 ST7796 单 GRAM 带来的极短暂撕裂，但不能持续异常；
4. 连续绘制 2 分钟，界面不逐渐卡顿；
5. 打开和关闭标签菜单，确认关闭路径回到右下角按钮；
6. 在标签间往返切换，确认正向向右进入、返回向左进入；
7. 点击菜单外区域，确认菜单收回且不会误切换页面；
8. 完成坏块网格覆盖并重置；
9. 连续点击移动靶点，确认统计更新且触摸坐标对齐；
10. 在设置页切换线宽和坐标显示，再重置全部测试。

实机视觉结果未确认前，不得标记为通过。
