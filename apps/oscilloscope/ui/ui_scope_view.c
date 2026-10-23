/*
 * Copyright 2026
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ui_scope_view.h"

#include <string.h>

#include "ui_theme.h"

#define SCOPE_WIDTH_PX     464
#define SCOPE_HEIGHT_PX    208
#define SCOPE_STRIDE_BYTES (SCOPE_WIDTH_PX / 8)

static uint8_t s_canvas_buffer[
    LV_CANVAS_BUF_SIZE(SCOPE_WIDTH_PX, SCOPE_HEIGHT_PX, 1, 1)]
    LV_ATTRIBUTE_MEM_ALIGN;
static lv_obj_t *s_canvas;
static ui_scope_mode_t s_mode;

static void scope_pixel(int32_t x, int32_t y, bool on)
{
    uint32_t byte_index;
    uint8_t mask;

    if ((x < 0) || (x >= SCOPE_WIDTH_PX) ||
        (y < 0) || (y >= SCOPE_HEIGHT_PX))
    {
        return;
    }

    byte_index = (uint32_t)y * SCOPE_STRIDE_BYTES + ((uint32_t)x >> 3U);
    mask = (uint8_t)(0x80U >> (x & 7));
    if (on)
    {
        s_canvas_buffer[byte_index] |= mask;
    }
    else
    {
        s_canvas_buffer[byte_index] &= (uint8_t)~mask;
    }
}

static void scope_line(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    int32_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int32_t sx = (x0 < x1) ? 1 : -1;
    int32_t dy = -((y1 > y0) ? (y1 - y0) : (y0 - y1));
    int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t error = dx + dy;

    for (;;)
    {
        scope_pixel(x0, y0, true);
        if ((x0 == x1) && (y0 == y1))
        {
            break;
        }
        if ((2 * error) >= dy)
        {
            error += dy;
            x0 += sx;
        }
        if ((2 * error) <= dx)
        {
            error += dx;
            y0 += sy;
        }
    }
}

static void scope_draw_grid(void)
{
    int32_t x;
    int32_t y;

    for (x = 0; x < SCOPE_WIDTH_PX; x += SCOPE_WIDTH_PX / 8)
    {
        for (y = 0; y < SCOPE_HEIGHT_PX; y += 4)
        {
            scope_pixel(x, y, true);
        }
    }
    for (y = 0; y < SCOPE_HEIGHT_PX; y += SCOPE_HEIGHT_PX / 4)
    {
        for (x = 0; x < SCOPE_WIDTH_PX; x += 4)
        {
            scope_pixel(x, y, true);
        }
    }
}

static uint32_t scope_trigger_index(const osc_snapshot_t *snapshot)
{
    uint32_t index;
    uint32_t threshold =
        ((uint32_t)snapshot->minimum_raw + snapshot->maximum_raw) / 2U;
    uint32_t last_start = OSC_WAVE_POINTS - SCOPE_WIDTH_PX;

    for (index = 1U; index < OSC_WAVE_POINTS; index++)
    {
        if ((snapshot->waveform[index - 1U] < threshold) &&
            (snapshot->waveform[index] >= threshold))
        {
            return (index < last_start) ? index : last_start;
        }
    }
    return last_start;
}

static void scope_draw_waveform(const osc_snapshot_t *snapshot)
{
    uint32_t start = scope_trigger_index(snapshot);
    uint32_t available = OSC_WAVE_POINTS - start;
    int32_t previous_y = 0;
    int32_t x;

    for (x = 0; x < SCOPE_WIDTH_PX; x++)
    {
        uint32_t sample_index =
            start + ((uint32_t)x * (available - 1U)) /
                (SCOPE_WIDTH_PX - 1U);
        uint32_t raw = snapshot->waveform[sample_index];
        int32_t y = (SCOPE_HEIGHT_PX - 1) -
            (int32_t)((raw * (SCOPE_HEIGHT_PX - 1U)) / 4095U);

        if (0 == x)
        {
            previous_y = y;
        }
        scope_line(x - 1, previous_y, x, y);
        previous_y = y;
    }
}

static void scope_draw_spectrum(const osc_snapshot_t *snapshot)
{
    int32_t x;

    for (x = 0; x < SCOPE_WIDTH_PX; x++)
    {
        uint32_t first_bin =
            ((uint32_t)x * (OSC_SPECTRUM_BINS - 1U)) /
                (SCOPE_WIDTH_PX - 1U);
        uint32_t last_bin =
            ((uint32_t)(x + 1) * (OSC_SPECTRUM_BINS - 1U)) /
                (SCOPE_WIDTH_PX - 1U);
        uint32_t magnitude = 0U;
        uint32_t bin;
        int32_t height;
        int32_t y;

        if (last_bin < first_bin)
        {
            last_bin = first_bin;
        }
        for (bin = first_bin; bin <= last_bin; bin++)
        {
            if (snapshot->spectrum[bin] > magnitude)
            {
                magnitude = snapshot->spectrum[bin];
            }
        }
        height = (int32_t)(
            (magnitude * (SCOPE_HEIGHT_PX - 1U)) / 1000U);
        for (y = SCOPE_HEIGHT_PX - 1;
             y >= SCOPE_HEIGHT_PX - 1 - height;
             y--)
        {
            scope_pixel(x, y, true);
        }
    }
}

void wd_scope_view_init(lv_obj_t *parent)
{
    s_mode = UI_SCOPE_MODE_WAVEFORM;
    s_canvas = lv_canvas_create(parent);
    lv_obj_remove_style_all(s_canvas);
    lv_canvas_set_buffer(
        s_canvas,
        s_canvas_buffer,
        SCOPE_WIDTH_PX,
        SCOPE_HEIGHT_PX,
        LV_COLOR_FORMAT_I1);
    lv_canvas_set_palette(s_canvas, 0U, ui_theme_scope_palette(0U));
    lv_canvas_set_palette(s_canvas, 1U, ui_theme_scope_palette(1U));
    lv_obj_set_pos(s_canvas, 8, 44);
    lv_obj_remove_flag(s_canvas, LV_OBJ_FLAG_SCROLLABLE);
}

void wd_scope_view_set_mode(ui_scope_mode_t mode)
{
    s_mode = mode;
}

void wd_scope_view_render(const osc_snapshot_t *snapshot, bool valid)
{
    memset(s_canvas_buffer, 0, sizeof(s_canvas_buffer));
    scope_draw_grid();
    if (valid)
    {
        if (UI_SCOPE_MODE_WAVEFORM == s_mode)
        {
            scope_draw_waveform(snapshot);
        }
        else
        {
            scope_draw_spectrum(snapshot);
        }
    }
    lv_obj_invalidate(s_canvas);
}
