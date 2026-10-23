# Dual-Core UI Boundary

## Recommended Ownership

### `cm33_core0`: UI Core

- LVGL, draw buffers and GUI state;
- ST7796S and FlexIO;
- GT911 and touch input;
- backlight, LCD reset, TE;
- UI animation and page transitions;
- lightweight UI logging;
- IPC endpoint for commands and status.

### `cm33_core1`: Functional Core

- motor/control loops;
- ADC, SINC, PWM and encoder;
- CAN and Ethernet;
- sensor acquisition;
- networking, files, AI or security workloads;
- IPC endpoint for commands and status.

Only core0 may access the LCD and touch hardware. Only the assigned core may
initialize or reconfigure a shared peripheral.

## IPC Selection

| Need | Preferred mechanism |
|---|---|
| Doorbell/event | Mailbox |
| Short command/reply | RPMsg-Lite or eRPC |
| Periodic UI status | Shared-memory snapshot with sequence |
| Mutual exclusion | SEMA42 |
| Large payload | Shared ring buffer / external storage |

Do not send a message for every touch sample or every PWM cycle. UI status can
normally update at 10–50 Hz independent of the 60 FPS render cadence.

## Shared State Pattern

```c
typedef struct {
    uint32_t sequence;
    uint32_t version;
    int32_t  value;
    uint32_t flags;
} ui_status_snapshot_t;
```

Writer:

1. increment sequence to an odd value;
2. write payload;
3. increment sequence to an even value.

Reader:

1. read sequence;
2. copy payload;
3. re-read sequence;
4. accept only matching even sequences.

Use compiler/CPU barriers and a documented memory region.

## Build Strategy

Start from:

```text
examples/multicore_examples/multicore_manager/primary
examples/multicore_examples/rpmsg_lite_pingpong
examples/driver_examples/sema42/dual_core/primary
```

Use `--sysbuild` for coupled images and inspect the generated runners file.
Do not build two independent projects and flash them arbitrarily over the same
flash regions.

## Resource Rules

- reserve separate stacks for each core;
- reserve LVGL heap and both draw buffers on core0;
- place function-specific buffers on core1;
- define the shared-memory range in both linker maps;
- publish, version and verify IPC layout definitions;
- keep core0 below its worst-case timing budget at the target FPS;
- test core1 deadlines while the UI performs a fast drag.

## Debug

LinkServer exposes both cores:

```bash
LinkServer gdbserver MCXN947:FRDM-MCXN947 -c all
```

Verify:

- core1 starts only once;
- shared clock/pin initialization is not duplicated;
- resetting one core does not leave stale shared state;
- a paused core does not permanently deadlock the other;
- UI rendering remains responsive during core1 load.
