# Touchscreen Tester Performance Record

## Baseline

- Date: 2026-10-23
- Board: FRDM-MCXN947
- Display: LCD-PAR-S035 / ST7796S, 480x320 landscape
- Touch: GT911
- UI core: `cm33_core0`
- SDK: MCUXpresso SDK 26.09.00
- LVGL: 9.4.0
- Toolchain: Arm GNU Toolchain 14.2.Rel1, GCC 14.2.1
- Performance Skill: `frdm-mcxn947-lvgl-ui-performance`

## Accepted Display Architecture

| Metric | Value |
|---|---:|
| Render mode | `LV_DISPLAY_RENDER_MODE_PARTIAL` |
| Pixel format | RGB565 |
| Partial stripe height | 80 rows |
| Bytes per stripe | 76,800 |
| Stripe buffers | 2, total 153,600 bytes |
| TE mode | V-blank |
| TE waits | Once per complete LVGL update |
| Buffer handoff | Non-blocking SmartDMA completion |
| UI period | 16 ms, nominal 62.5 FPS |

## Static Evidence

```text
m_interrupts:         688 B
m_text:           382,168 B
m_data:           275,880 B / 312 KiB
static RAM:       275,888 B
audit failures:         0
```

The configuration audit passes buffer units, dual-buffer setup, single-TE batching,
VBlank mode, callback guarding, non-blocking flush, write-error handling and the
16 ms UI period.

## Flash Evidence

The image was flashed through LinkServer to the connected FRDM-MCXN947. A wire
reset produced exactly the expected startup line without a reset loop:

```text
Touchscreen tester started: 480x320, 16 ms UI cadence
```

## Physical Visual Evidence

Pending user observation:

- static rendering;
- slow and fast free drawing;
- two-minute continuous drawing;
- menu open/close return path;
- forward and backward page transitions;
- bad-block grid coverage;
- accuracy target movement and statistics;
- recovery immediately after motion stops.

## Decision

Build, link, flash, startup and configuration gates pass. Physical interaction
and visual smoothness remain pending until the user confirms them on the panel.
