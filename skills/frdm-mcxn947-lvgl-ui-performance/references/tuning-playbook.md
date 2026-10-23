# Tuning Playbook

## 1. Classify Before Tuning

| Symptom | Likely cause | First check |
|---|---|---|
| Diagonal or horizontal boundary while dragging | Panel scan vs GRAM writes | TE, buffer batching, refresh rate |
| Only fast drag tears | Update area exceeds practical display window | Coalesce updates, larger stripe, accept single-GRAM limit |
| Static image corrupted | Wrong buffer size, stride, bus timing or data corruption | LVGL buffer bytes, map file, logic analyzer |
| Touch works but display stalls | DMA/LVGL handoff or reset loop | DMA callback, `flush_ready`, fault registers |
| Drag starts smooth then slows | Every strip waits for a new frame | One TE wait per LVGL update |
| Sudden reset | NULL callback, stack/RAM overflow, HardFault | Debugger fault backtrace, map file |

Do not treat every visual defect as tearing. Persistent corruption is usually
not a VSync problem.

## 2. Resource Matrix

| Technique | RAM | CPU | Bus | Tearing impact |
|---|---:|---:|---:|---|
| More frequent TE waits | unchanged | higher | higher | may worsen latency |
| Larger partial stripe | higher | slightly lower overhead | fewer transactions | usually reduces seams |
| Two async buffers | higher only if size grows | overlapped render/transfer | better utilization | helps consistency |
| 30 FPS cap | unchanged | lower | lower | fewer transitions, less responsive |
| 60 FPS cap | unchanged | higher | higher | more responsive, not a tear cure |
| Full frame buffer | ~300 KiB per frame | high draw bandwidth | high | needs atomic display support to cure tearing |
| External RAM/dual-GRAM display | higher BOM | lower MCU pressure | different | required for true tear-free arbitrary updates |

## 3. Buffer Sizing

For 480x320 RGB565:

| Stripe height | Bytes per buffer | Two buffers | Practical note |
|---:|---:|---:|---|
| 32 | 30,720 | 61,440 | many strips, more tearing opportunities |
| 64 | 61,440 | 122,880 | moderate |
| 80 | 76,800 | 153,600 | current baseline |
| 160 | 153,600 | 307,200 | likely too much with LVGL heap and app data |
| 320 | 307,200 | 614,400 | impossible with current MCU memory |

Always pass `sizeof(buffer)` or an explicit byte count to
`lv_display_set_buffers()`. A pixel count is a correctness bug.

## 4. TE Strategy

The known-good strategy is:

1. Enable `kST7796s_TEVSync`.
2. Wait once at the first flush of a complete LVGL update.
3. Send all strips belonging to that update.
4. Re-arm the TE wait after the last strip is scheduled.
5. Use a timeout so missing TE does not deadlock the UI.

Do not wait once per strip. That serializes the UI at one strip per display
frame and causes long drag latency.

TE only aligns the beginning of a transfer. If the transfer lasts longer than
the useful blanking interval, the panel can still scan and tear. This is a
fundamental single-GRAM limitation of ST7796S.

## 5. Async Double Buffering

LVGL 9 already controls two buffers. A flush callback may return before DMA
completion, but it must call `lv_display_flush_ready()` exactly when the active
buffer becomes reusable.

Requirements:

- store the display pointer after `lv_display_create()`;
- ignore completion callbacks before that pointer exists (initial clear);
- release every completion, not only the final strip;
- check `ST7796S_WritePixels()` return values;
- never start a second transfer while the LCD bus is busy;
- keep buffer ownership and TE timing explicit.

Async double buffering improves CPU/DMA overlap. It does not create a second
panel GRAM and cannot guarantee zero tearing.

## 6. Refresh Rate

The source defines a 1 ms SysTick and `LVGL_TASK_PERIOD_TICK` controls UI
processing cadence:

| Ticks | Nominal rate | Tradeoff |
|---:|---:|---|
| 5 | 200 Hz | excessive for this panel |
| 16 | 62.5 FPS | current 60 FPS target |
| 17 | 58.8 FPS | closer to 60 Hz panel |
| 20 | 50 FPS | smoother resource compromise |
| 33 | 30 FPS | lower CPU/bus, less responsive |

Always measure actual frame time; the configured cap is not achieved FPS.

## 7. Reduce Invalidated Area

For drag-heavy UIs:

- avoid shadows, blur and expensive transparency on moving objects;
- avoid full-screen background invalidation;
- update the object position once per UI refresh, not for every raw touch event;
- move a lightweight foreground layer rather than invalidating the whole page;
- keep static backgrounds in a separate non-scrolling layer if the LVGL version
  supports it;
- avoid changing style properties every frame;
- avoid serial logging in the flush path.

These are performance changes. Treat visual redesign as a separate style-skill
decision.

## 8. Escalation

Use this order:

1. fix correctness and memory units;
2. reduce unnecessary invalidation;
3. TE once per update;
4. enlarge stripe within RAM budget;
5. pipeline two stripes;
6. cap refresh to product need;
7. move functional work to core1;
8. change display hardware only if arbitrary fast motion must be tear-free.

For ST7796S, brief tearing during very fast moving content can remain even after
correct synchronization. State this hardware limit instead of layering
unrelated hacks.
