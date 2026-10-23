# Version Lock

This repository does not vendor the MCUXpresso SDK, compiler, debugger, or vendor
documentation. Reproduce the environment using the versions below.

| Component | Version or revision |
|---|---|
| Repository baseline | first project commit |
| MCUXpresso SDK | 26.09.00 |
| SDK manifests revision | `0a103dfa35e7d6ae1eabf516488259fb9b423075` |
| MCUXpresso SDK core revision | `c6f4223f45fdab59509f25ee3ea63a71bd29d8aa` |
| Arm GNU Toolchain | 14.2.Rel1, GCC 14.2.1 |
| west | 1.5.0 |
| CMake | 3.31.10 |
| Ninja | 1.13.2 |
| LinkServer | 26.9.130 |
| MCUXpresso for VS Code | 26.9.44 |
| LVGL | 9.4.0 |
| Board | FRDM-MCXN947 |
| MCU-Link serial | `GTELBKOY3HRDG` |
| LCD module | LCD-PAR-S035 |
| Panel | HP35001-DCT |
| Display controller | ST7796U/ST7796S compatible |
| Touch controller | GT911 |

## Build Verification

```text
build/hello_world_core0/hello_world_cm33_core0.elf
build/lvgl_s035_core0/lvgl_guider_bm_cm33_core0.elf
```

The LVGL firmware was flashed to the connected FRDM-MCXN947 and ran the GUI.
Display tearing was observed while dragging the blue widget. The root-cause
investigation and subsequent display optimization are tracked separately.

