# Platform Baseline

## Hardware

| Item | Value |
|---|---|
| Board | NXP FRDM-MCXN947 |
| MCU | MCXN947VDFT, dual Cortex-M33 at up to 150 MHz |
| MCU-Link serial | Read from the probe; it is workstation-specific |
| MCU-Link firmware | CMSIS-DAP V3.128 |
| Display module | LCD-PAR-S035 |
| Panel | HP35001-DCT, 3.5-inch IPS |
| Resolution | 320x480 native, 480x320 landscape in the UI |
| Display controller | ST7796U, ST7796S-compatible driver |
| Touch controller | Goodix GT911, 5-point |
| Display interface | FRDM J8, 16-bit 8080, FlexIO |
| Touch interface | I2C2 on P4_0/P4_1, INT on P4_6 |
| TE | P0_13 |
| Backlight | P4_5 control, nominal 3.1 V/120 mA |

## Software

| Item | Value |
|---|---|
| SDK | MCUXpresso SDK 26.09.00 |
| SDK manifests | `0a103dfa35e7d6ae1eabf516488259fb9b423075` |
| SDK core | `c6f4223f45fdab59509f25ee3ea63a71bd29d8aa` |
| Toolchain | Arm GNU Toolchain 14.2.Rel1, GCC 14.2.1 |
| LVGL | 9.4.0 |
| CMake | 3.31.10 |
| Ninja | 1.13.2 |
| LinkServer | 26.9.130 |
| Board target | `frdmmcxn947` |
| Core | `cm33_core0` |

## Baseline Application

```text
mcuxsdk/examples/lvgl_examples/lvgl_guider_bm
```

Current UI:

- screen: 480x320 landscape;
- blue rectangle: 40% by 40%, about 192x128 pixels;
- label: `Drag Me`;
- drag event: `LV_EVENT_PRESSING`, adds the current touch vector to the object's
  aligned X/Y position.

The large moving object invalidates both old and new areas. This makes it a
useful stress test for tearing, bus throughput and refresh latency.

## Known-Good Display Shape

```text
render mode: LV_DISPLAY_RENDER_MODE_PARTIAL
pixel format: RGB565
stripe height: LCD_HEIGHT / 4 = 80 rows
stripe bytes: 480 * 80 * 2 = 76,800
two stripe buffers: 153,600 bytes
TE mode: kST7796s_TEVSync
TE wait: once per complete LVGL update
buffer handoff: asynchronous, SmartDMA completion
UI cadence: 16 ticks = 16 ms, nominal 62.5 FPS
```

Current link result:

```text
m_text: 313 KB / 767 KB
m_data: 211 KB / 312 KB
```

Do not infer that 60 FPS is always achieved. The panel scan is about 60 Hz and
the MCU still has to select an area and write pixels through FlexIO.

## Empirical History

| Change | Observation |
|---|---|
| LVGL API passed pixels instead of bytes | Effective stripe was 16 rows, not 32 |
| TE disabled | Diagonal tearing during drag |
| Wait for TE on every strip | Horizontal tearing and severe drag lag |
| One TE wait per LVGL update, 1/4 buffers | Tearing clearly reduced |
| TE H/V mode | Did not improve enough; later superseded |
| Async double-buffer pipeline | Slow drag nearly clean; very fast drag may still tear |
| 30 FPS | Lower resource use but less responsive |
| 60 FPS | Better responsiveness; higher CPU/bus load |

## Verified Failure

An early async implementation called `lv_display_flush_ready()` with a NULL
display pointer during the initial LCD clear. This caused HardFault and a reset
loop. The callback must be disabled until `s_display` is assigned.

## Current User-Observed Behavior

On the current accepted baseline:

- UI and touch work;
- normal/slow dragging has little visible tearing;
- fast dragging can still show brief tearing;
- early async versions that lacked the display guard caused reboot/startup
  instability;
- the current build must be checked for the guard and for `s_memWriteDone`
  handling before accepting a new patch.

## Source Of Truth

- Data sheet and board manual in the project `docs/` directory.
- LCD-PAR-S035 schematic and panel spec in `../LCD-PAR-S035`.
- `firmware/patches/` contains project-specific SDK changes.
- `firmware/README.md` documents patch order and rationale.
- Top-level Git history records accepted project states.
