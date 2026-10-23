---
name: frdm-mcxn947-lvgl-ui-performance
description: "Optimize and validate LVGL UI performance on FRDM-MCXN947/MCXN947 with MCUXpresso SDK, ST7796S, GT911, FlexIO and SmartDMA, including tearing, partial buffering, TE synchronization, refresh cadence and dual-core ownership. Use for performance or correctness work, not visual style design."
---

# FRDM-MCXN947 LVGL UI Performance

Use this skill whenever implementing, reviewing, or optimizing LVGL UI rendering
on FRDM-MCXN947 with the LCD-PAR-S035/ST7796S display. It protects the known-good
refresh architecture and provides a reproducible procedure for diagnosing
tearing, frame stalls and excessive resource use.

This skill does not define colors, typography, spacing, component styling or
visual hierarchy. When a future UI style skill exists, apply this performance
skill for implementation and acceptance, and apply the style skill for
appearance. If a style change conflicts with timing or memory limits, preserve
correctness and report the tradeoff.

## Required Context

Read [references/platform-baseline.md](references/platform-baseline.md) before
changing display code. It records the verified hardware/software baseline and
known failure modes.

Use these references only when the task reaches the relevant branch:

- [references/tuning-playbook.md](references/tuning-playbook.md): tearing,
  latency, throughput, RAM and refresh-rate decisions.
- [references/dual-core-ui.md](references/dual-core-ui.md): splitting UI and
  functional workloads across `cm33_core0`/`cm33_core1`.
- [references/acceptance-tests.md](references/acceptance-tests.md): mandatory
  build, flash, visual, runtime and resource checks.

## Non-Negotiable Invariants

1. One display owner. Only the UI core may access FlexIO, ST7796S, LCD control
   pins, backlight or GT911.
2. Derive buffer sizes from bytes. LVGL 9 `lv_display_set_buffers()` expects a
   byte count, not a pixel count.
3. Prefer two partial RGB565 buffers at about one quarter of screen height.
   Do not propose full-frame double buffering for 480x320; two frames exceed the
   practical MCU memory budget.
4. Keep VBlank TE synchronization as the baseline. Wait once per complete LVGL
   update, not once per partial strip.
5. If buffers are pipelined, release every completed buffer from the DMA
   callback. Never wait until only the final strip completes.
6. The DMA callback can run during pre-LVGL LCD clearing. Guard notification
   with `s_display != NULL`.
7. Every error path that starts a flush must either complete it or call
   `lv_display_flush_ready()`.
8. Do not put blocking logs, file I/O, network calls or long computations in the
   flush callback or TE wait path.
9. Do not claim visual improvement from build success alone. The developer
   cannot see the panel; require the user to confirm slow drag, fast drag,
   static recovery and perceived smoothness.
10. Change one performance variable per experiment unless the current code is
    already known broken. Record the result and commit it as a patch.

## Reproduction Commands

Run the audit before and after each change:

```bash
bash skills/frdm-mcxn947-lvgl-ui-performance/scripts/audit_ui_config.sh
```

Build and optionally flash:

```bash
bash skills/frdm-mcxn947-lvgl-ui-performance/scripts/build_ui.sh
bash skills/frdm-mcxn947-lvgl-ui-performance/scripts/build_ui.sh --flash
```

The default app is `lvgl_guider_bm` on `cm33_core0`. Override with `UI_APP`,
`UI_BUILD_DIR`, `UI_CONFIG`, `UI_CORE` or `MCXN_SDK` when testing another project.

## Workflow

### 1. Establish the baseline

- Identify display controller, touch controller, interface, orientation and
  resolution.
- Record SDK/toolchain revisions and current applied patches.
- Record static RAM and Flash from the link result.
- Record observed behavior: static UI, slow drag, fast drag, page transition,
  long-duration drag, resets and serial health.
- Confirm whether the issue is tearing, stale pixels, coordinate error, bus
  corruption, starvation or a crash. Do not tune before classifying it.

### 2. Budget before changing buffers

Calculate:

```text
frame_bytes = width * height * bytes_per_pixel
stripe_bytes = width * stripe_height * bytes_per_pixel
bandwidth = frame_bytes * target_fps
```

For 480x320 RGB565:

- full frame: 307,200 bytes;
- two full frames: 614,400 bytes, not viable here;
- two 1/4-height stripes: 153,600 bytes total.

Check `m_data`, stack, heap and other-core RAM before increasing stripe size.

### 3. Select a small experiment

Use the decision matrix in
[references/tuning-playbook.md](references/tuning-playbook.md). Typical order:

1. Fix unit errors or incorrect memory ownership.
2. Synchronize to TE.
3. Batch all strips of one LVGL update behind one TE wait.
4. Increase partial stripe size if RAM permits.
5. Pipeline two buffers asynchronously.
6. Cap UI cadence to the product requirement.
7. Reduce invalidated area or improve UI update coalescing.
8. Split functional work onto core1 when core0 is saturated.

Do not use TE-H/V mode, full-frame buffering, lower color depth or a refresh
cap as universal defaults. Each is an experiment with measurable tradeoffs.
Record each experiment with the template in
[references/acceptance-tests.md](references/acceptance-tests.md#experiment-record-template).

### 4. Implement through a tracked patch

The SDK workspace is ignored by Git. Preserve every project-specific source
change as a numbered patch under `firmware/patches/` and document it in
`firmware/README.md`.

Before editing, inspect current Git and SDK state:

```bash
git -C /home/jxy/NXP/MCXN947 status --short --branch
git -C /home/jxy/NXP/MCXN947/mcuxpresso-sdk/mcuxsdk/examples status --short
```

Do not overwrite unrelated SDK changes. Verify patch fidelity against the
working tree where practical.

### 5. Build, flash and observe

- Run `audit_ui_config.sh`.
- Build with `build_ui.sh`.
- Flash only to the connected FRDM-MCXN947.
- Check serial for HardFault/reset loops and expected periodic output.
- Ask the user to test the same interaction as the baseline.
- Compare both tearing and latency; an optimization can reduce one and worsen
  the other.

### 6. Commit and preserve the accepted state

- Add or update the patch file.
- Record measured memory/CPU effects and visual observations.
- Commit the experiment.
- If rejected, restore the last accepted patch sequence and document why.

## Current Accepted Performance Shape

Unless a task proves otherwise, start from this known-good combination:

```text
LCD:        LCD-PAR-S035 / ST7796S, 480x320 landscape
Touch:      GT911
Bus:        FlexIO 16-bit 8080 + SmartDMA
Render:     LVGL PARTIAL, RGB565
Buffers:    two 480x80 stripes (76,800 bytes each)
TE:         V-blank, one wait per complete LVGL update
Pipeline:   asynchronous buffer release from SmartDMA completion
UI cadence: 16 ms, nominal 62.5 FPS; actual <= panel/TE capability
```

Current empirical result: slow dragging is nearly clean; very fast dragging may
retain brief tearing because ST7796 has only one internal GRAM. Do not regress
the accepted path while pursuing a theoretical zero-tearing result.

## Required User Feedback

After flashing, ask specifically about:

1. static rendering;
2. slow drag;
3. fast drag;
4. long-duration drag and stutter;
5. whether artifacts disappear immediately after motion stops.

If the user cannot see the panel, leave visual acceptance pending. Build and
serial evidence cannot substitute for visual confirmation.

## Style Boundary

For UI work, separate two questions:

- performance/correctness: this skill;
- appearance and design language: the future style skill.

Performance may require removing expensive effects or reducing invalidation
area, but do not silently redesign the interface. State the measurable reason
and request confirmation when a visual compromise is required.
