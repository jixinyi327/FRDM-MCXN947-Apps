# Firmware Overlays

The MCUXpresso SDK is fetched separately and ignored by the top-level repository.
Project-specific fixes are kept as patches so the SDK remains reproducible.

## Target

- Board: FRDM-MCXN947
- SDK: 26.09.00
- Example: `examples/lvgl_examples/lvgl_guider_bm`
- Display: LCD-PAR-S035 / ST7796S
- Touch: GT911

## Apply display optimization

```bash
source /home/jxy/NXP/MCXN947/开发计划/scripts/env.sh

git -C "$MCXN_SDK/mcuxsdk/examples" apply \
  /home/jxy/NXP/MCXN947/firmware/patches/0001-fix-lvgl-partial-buffer-size.patch
```

## Build and flash

```bash
cd "$MCXN_SDK"

west build -b frdmmcxn947 \
  mcuxsdk/examples/lvgl_examples/lvgl_guider_bm \
  -d build/lvgl_s035_core0 -p always \
  --config debug -Dcore_id=cm33_core0

west flash -d build/lvgl_s035_core0 -r linkserver
```

## Revision 0001: LVGL partial buffer units

LVGL 9 expects the `lv_display_set_buffers` size argument in bytes. The upstream
MCUXpresso adapter passed the number of pixels, causing the intended 32-row
partial buffer to be treated as a 16-row buffer and doubling the number of
display-memory updates.

The patch keeps the same allocation and RAM usage, but restores the intended
32-row buffer height.

## Revision 0002: TE/VBlank synchronization

The second patch enables the panel TE signal in V-blank mode and waits for the
next complete TE pulse before writing pixels to ST7796S internal memory. A
25 ms timeout prevents a missing TE signal from locking the UI permanently.

Apply in order:

```bash
git -C "$MCXN_SDK/mcuxsdk/examples" apply \
  /home/jxy/NXP/MCXN947/firmware/patches/0001-fix-lvgl-partial-buffer-size.patch

git -C "$MCXN_SDK/mcuxsdk/examples" apply \
  /home/jxy/NXP/MCXN947/firmware/patches/0002-sync-display-flush-to-te.patch
```

## Revision 0003: one TE wait per LVGL update

Waiting for TE before every partial-strip write capped the UI at roughly one
strip per display frame and caused long drags to become slow. This revision:

- waits only on the first flush of each complete LVGL update;
- writes all strips for that update immediately after the TE pulse;
- increases the partial buffer from 1/10 to 1/4 of the screen height;
- restores the TE-wait state on the last flush of the update.

Apply all three patches in numeric order.



