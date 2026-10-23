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
m_text:           382,760 B
m_data:           276,264 B / 312 KiB
static RAM:       276,272 B
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

## Footer Layout Revision

- Replaced the 58x58 circular TAB control with a 72x36 rectangular TAB control;
- added an equally sized Reset button at the bottom-left;
- added a 296x36 information bar between the buttons;
- moved page status from page-local labels into per-page information strings;
- reused the existing Double buffer and SmartDMA path without changing timing;
- measured static RAM increase: 384 bytes;
- build, audit, flash and startup checks pass;
- physical footer alignment and information readability remain pending user
  confirmation.

## Physical Visual Evidence

Pending user observation:

- static rendering;
- slow and fast free drawing;
- two-minute continuous drawing;
- footer alignment, equal button sizes and information updates;
- menu open/close return path;
- forward and backward page transitions;
- bad-block grid coverage;
- accuracy target movement and statistics;
- recovery immediately after motion stops.

## Decision

Build, link, flash, startup and configuration gates pass. Physical interaction
and visual smoothness remain pending until the user confirms them on the panel.
