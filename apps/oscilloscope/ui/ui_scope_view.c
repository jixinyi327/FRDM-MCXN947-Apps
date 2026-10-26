/*
 * Copyright 2026
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ui_scope_view.h"

#include "ui_theme.h"

#define SCOPE_WIDTH_PX  400
#define SCOPE_HEIGHT_PX 184
#define SCOPE_POINTS    464
#define SCOPE_WINDOW_POINTS 1536U
#define SCOPE_MID_Y     ((SCOPE_HEIGHT_PX - 1) / 2)
#define SCOPE_X_PX      72
#define SCOPE_Y_PX      44

static lv_point_precise_t s_wave_points[SCOPE_POINTS];
static lv_point_precise_t s_spectrum_points[SCOPE_POINTS];
static lv_obj_t *s_plot;
static lv_obj_t *s_wave_line;
static lv_obj_t *s_spectrum_line;
static lv_obj_t *s_y_top_label;
static lv_obj_t *s_y_middle_label;
static lv_obj_t *s_y_bottom_label;
static lv_obj_t *s_x_tick_labels[5];
static lv_obj_t *s_x_minor_ticks[16];
static lv_obj_t *s_x_key_ticks[5];
static lv_obj_t *s_y_minor_ticks[8];
static lv_obj_t *s_y_key_ticks[3];
static ui_scope_mode_t s_mode;
static ui_scope_mode_t s_axis_mode;
static uint32_t s_axis_sample_rate;

static lv_obj_t *scope_create_axis_label(
    lv_obj_t *parent, const char *text)
{
    lv_obj_t *label = lv_label_create(parent);

    lv_label_set_text(label, text);
    ui_style_apply_scope_axis_label(label);
    lv_obj_set_style_text_font(
        label, ui_theme_axis_font(), LV_PART_MAIN);
    return label;
}

static lv_obj_t *scope_create_tick(
    lv_obj_t *parent, int32_t width, int32_t height)
{
    lv_obj_t *tick = lv_obj_create(parent);

    ui_style_apply_scope_tick(tick);
    lv_obj_set_size(tick, width, height);
    lv_obj_remove_flag(tick, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(tick, LV_OBJ_FLAG_CLICKABLE);
    return tick;
}

static void scope_position_axis_labels(void)
{
    if (NULL == s_plot)
    {
        return;
    }

    lv_obj_align_to(
        s_y_top_label, s_plot, LV_ALIGN_OUT_LEFT_TOP, -14, 0);
    lv_obj_align_to(
        s_y_middle_label, s_plot, LV_ALIGN_OUT_LEFT_MID, -14, 0);
    lv_obj_align_to(
        s_y_bottom_label, s_plot, LV_ALIGN_OUT_LEFT_BOTTOM, -14, 0);

    lv_obj_align_to(
        s_x_tick_labels[0], s_plot, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 10);
    lv_obj_align_to(
        s_x_tick_labels[1], s_plot, LV_ALIGN_OUT_BOTTOM_LEFT,
        SCOPE_WIDTH_PX / 4, 10);
    lv_obj_align_to(
        s_x_tick_labels[2], s_plot, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
    lv_obj_align_to(
        s_x_tick_labels[3], s_plot, LV_ALIGN_OUT_BOTTOM_LEFT,
        (SCOPE_WIDTH_PX * 3) / 4, 10);
    lv_obj_align_to(
        s_x_tick_labels[4], s_plot, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 10);

    lv_obj_align_to(
        s_x_key_ticks[0], s_plot, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
    lv_obj_align_to(
        s_x_key_ticks[1], s_plot, LV_ALIGN_OUT_BOTTOM_LEFT,
        SCOPE_WIDTH_PX / 4, 0);
    lv_obj_align_to(
        s_x_key_ticks[2], s_plot, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);
    lv_obj_align_to(
        s_x_key_ticks[3], s_plot, LV_ALIGN_OUT_BOTTOM_LEFT,
        (SCOPE_WIDTH_PX * 3) / 4, 0);
    lv_obj_align_to(
        s_x_key_ticks[4], s_plot, LV_ALIGN_OUT_BOTTOM_RIGHT, 0, 0);

    lv_obj_align_to(
        s_y_key_ticks[0], s_plot, LV_ALIGN_OUT_LEFT_TOP, -4, 0);
    lv_obj_align_to(
        s_y_key_ticks[1], s_plot, LV_ALIGN_OUT_LEFT_MID, -4, 0);
    lv_obj_align_to(
        s_y_key_ticks[2], s_plot, LV_ALIGN_OUT_LEFT_BOTTOM, -4, 0);

    for (uint32_t index = 0U; index < 16U; index++)
    {
        uint32_t group = index / 4U;
        uint32_t position =
            group * 5U + (index % 4U) + 1U;

        lv_obj_align_to(
            s_x_minor_ticks[index],
            s_plot,
            LV_ALIGN_OUT_BOTTOM_LEFT,
            (SCOPE_WIDTH_PX * position) / 20U - 1,
            0);
    }

    for (uint32_t index = 0U; index < 8U; index++)
    {
        uint32_t tenth =
            (index < 4U) ? (index + 1U) : (index + 2U);

        lv_obj_align_to(
            s_y_minor_ticks[index],
            s_plot,
            LV_ALIGN_OUT_LEFT_TOP,
            -3,
            (SCOPE_HEIGHT_PX * tenth) / 10U - 1);
    }
}

static void scope_set_time_label(lv_obj_t *label, uint64_t time_ns)
{
    if (time_ns >= 1000000ULL)
    {
        lv_label_set_text_fmt(
            label,
            "%lu.%02lu ms",
            (unsigned long)(time_ns / 1000000ULL),
            (unsigned long)((time_ns % 1000000ULL) / 10000ULL));
    }
    else if (time_ns >= 1000ULL)
    {
        lv_label_set_text_fmt(
            label,
            "%lu us",
            (unsigned long)((time_ns + 500ULL) / 1000ULL));
    }
    else
    {
        lv_label_set_text_fmt(
            label, "%lu ns", (unsigned long)time_ns);
    }
}

static void scope_update_axis_labels(uint32_t sample_rate_hz)
{
    uint64_t span_ns;
    uint32_t frequency_hz;

    if (0U == sample_rate_hz)
    {
        sample_rate_hz = OSC_SAMPLE_RATE_HZ;
    }
    if ((sample_rate_hz == s_axis_sample_rate) &&
        (s_mode == s_axis_mode) && (NULL != s_x_tick_labels[4]))
    {
        return;
    }
    s_axis_sample_rate = sample_rate_hz;
    s_axis_mode = s_mode;

    if (UI_SCOPE_MODE_WAVEFORM == s_mode)
    {
        span_ns =
            ((uint64_t)(SCOPE_WINDOW_POINTS - 1U) * 1000000000ULL) /
            sample_rate_hz;
        for (uint32_t tick = 0U; tick < 5U; tick++)
        {
            scope_set_time_label(
                s_x_tick_labels[tick],
                (span_ns * tick) / 4U);
        }
        lv_label_set_text(s_y_top_label, "3.30 V");
        lv_label_set_text(s_y_middle_label, "1.65 V");
        lv_label_set_text(s_y_bottom_label, "0 V");
    }
    else
    {
        frequency_hz = sample_rate_hz / 2U;
        for (uint32_t tick = 0U; tick < 5U; tick++)
        {
            uint32_t tick_hz = (frequency_hz * tick) / 4U;

            if ((4U == tick) && (0U == (tick_hz % 1000U)))
            {
                lv_label_set_text_fmt(
                    s_x_tick_labels[tick],
                    "%lu MHz",
                    (unsigned long)(tick_hz / 1000000U));
            }
            else if (tick_hz >= 1000U)
            {
                lv_label_set_text_fmt(
                    s_x_tick_labels[tick],
                    "%lu kHz",
                    (unsigned long)(tick_hz / 1000U));
            }
            else
            {
                lv_label_set_text_fmt(
                    s_x_tick_labels[tick],
                    "%lu Hz",
                    (unsigned long)tick_hz);
            }
        }
        lv_label_set_text(s_y_top_label, "0 dBFS");
        lv_label_set_text(s_y_middle_label, "-40 dBFS");
        lv_label_set_text(s_y_bottom_label, "-80 dBFS");
    }
    scope_position_axis_labels();
}

static int32_t scope_raw_to_y(uint32_t raw)
{
    return (SCOPE_HEIGHT_PX - 1) -
        (int32_t)((raw * (SCOPE_HEIGHT_PX - 1U)) / 65535U);
}

static void scope_render_waveform(const osc_snapshot_t *snapshot)
{
    uint32_t point;

    for (point = 0U; point < SCOPE_POINTS; point++)
    {
        uint32_t sample_index =
            (point * (SCOPE_WINDOW_POINTS - 1U)) /
                (SCOPE_POINTS - 1U);
        s_wave_points[point].x =
            (int32_t)((point * (SCOPE_WIDTH_PX - 1U)) /
                      (SCOPE_POINTS - 1U));
        s_wave_points[point].y =
            scope_raw_to_y(snapshot->waveform[sample_index]);
    }
    lv_line_set_points(s_wave_line, s_wave_points, SCOPE_POINTS);
}

static void scope_render_spectrum(const osc_snapshot_t *snapshot)
{
    uint32_t point;

    for (point = 0U; point < SCOPE_POINTS; point++)
    {
        uint32_t first_bin =
            (point * (OSC_SPECTRUM_BINS - 1U)) / (SCOPE_POINTS - 1U);
        uint32_t last_bin =
            ((point + 1U) * (OSC_SPECTRUM_BINS - 1U)) /
                (SCOPE_POINTS - 1U);
        uint32_t magnitude = 0U;
        uint32_t bin;

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

        s_spectrum_points[point].x =
            (int32_t)((point * (SCOPE_WIDTH_PX - 1U)) /
                      (SCOPE_POINTS - 1U));
        s_spectrum_points[point].y = (SCOPE_HEIGHT_PX - 1) -
            (int32_t)((magnitude * (SCOPE_HEIGHT_PX - 1U)) / 1000U);
    }
    lv_line_set_points(s_spectrum_line, s_spectrum_points, SCOPE_POINTS);
}

static void scope_show_mode(ui_scope_mode_t mode)
{
    if (UI_SCOPE_MODE_WAVEFORM == mode)
    {
        lv_obj_remove_flag(s_wave_line, LV_OBJ_FLAG_HIDDEN);
        lv_obj_add_flag(s_spectrum_line, LV_OBJ_FLAG_HIDDEN);
    }
    else
    {
        lv_obj_add_flag(s_wave_line, LV_OBJ_FLAG_HIDDEN);
        lv_obj_remove_flag(s_spectrum_line, LV_OBJ_FLAG_HIDDEN);
    }
}

void wd_scope_view_init(lv_obj_t *parent)
{
    lv_obj_t *axis;
    uint32_t point;

    s_mode = UI_SCOPE_MODE_WAVEFORM;
    s_plot = lv_obj_create(parent);
    ui_style_apply_scope_plot(s_plot);
    lv_obj_set_pos(s_plot, SCOPE_X_PX, SCOPE_Y_PX);
    lv_obj_set_size(s_plot, SCOPE_WIDTH_PX, SCOPE_HEIGHT_PX);
    lv_obj_remove_flag(s_plot, LV_OBJ_FLAG_SCROLLABLE);

    axis = lv_obj_create(s_plot);
    ui_style_apply_scope_axis(axis);
    lv_obj_set_pos(axis, 0, SCOPE_MID_Y);
    lv_obj_set_size(axis, SCOPE_WIDTH_PX, 1);
    lv_obj_remove_flag(axis, LV_OBJ_FLAG_SCROLLABLE);

    for (point = 0U; point < SCOPE_POINTS; point++)
    {
        s_wave_points[point].x =
            (int32_t)((point * (SCOPE_WIDTH_PX - 1U)) /
                      (SCOPE_POINTS - 1U));
        s_wave_points[point].y = SCOPE_MID_Y;
        s_spectrum_points[point] = s_wave_points[point];
    }

    s_wave_line = lv_line_create(s_plot);
    ui_style_apply_scope_line(s_wave_line);
    lv_obj_set_pos(s_wave_line, 0, 0);
    lv_obj_set_size(s_wave_line, SCOPE_WIDTH_PX, SCOPE_HEIGHT_PX);
    lv_line_set_points(s_wave_line, s_wave_points, SCOPE_POINTS);

    s_spectrum_line = lv_line_create(s_plot);
    ui_style_apply_scope_line(s_spectrum_line);
    lv_obj_set_pos(s_spectrum_line, 0, 0);
    lv_obj_set_size(s_spectrum_line, SCOPE_WIDTH_PX, SCOPE_HEIGHT_PX);
    lv_line_set_points(s_spectrum_line, s_spectrum_points, SCOPE_POINTS);

    s_y_top_label = scope_create_axis_label(parent, "3.30 V");
    s_y_middle_label = scope_create_axis_label(parent, "1.65 V");
    s_y_bottom_label = scope_create_axis_label(parent, "0 V");
    for (point = 0U; point < 5U; point++)
    {
        s_x_tick_labels[point] =
            scope_create_axis_label(parent, "0");
    }
    for (point = 0U; point < 5U; point++)
    {
        s_x_key_ticks[point] = scope_create_tick(parent, 2, 8);
    }
    for (point = 0U; point < 3U; point++)
    {
        s_y_key_ticks[point] = scope_create_tick(parent, 8, 2);
    }
    for (point = 0U; point < 16U; point++)
    {
        s_x_minor_ticks[point] = scope_create_tick(parent, 2, 4);
    }
    for (point = 0U; point < 8U; point++)
    {
        s_y_minor_ticks[point] = scope_create_tick(parent, 4, 2);
    }
    scope_show_mode(s_mode);
    scope_update_axis_labels(OSC_SAMPLE_RATE_HZ);
}

void wd_scope_view_set_mode(ui_scope_mode_t mode)
{
    s_mode = mode;
    scope_show_mode(mode);
    scope_update_axis_labels(
        (0U == s_axis_sample_rate) ? OSC_SAMPLE_RATE_HZ :
            s_axis_sample_rate);
}

void wd_scope_view_render(const osc_snapshot_t *snapshot, bool valid)
{
    if (!valid)
    {
        scope_show_mode(s_mode);
        return;
    }

    if (UI_SCOPE_MODE_WAVEFORM == s_mode)
    {
        scope_render_waveform(snapshot);
    }
    else
    {
        scope_render_spectrum(snapshot);
    }
    scope_update_axis_labels(snapshot->sample_rate_hz);
}
