/*
 * Copyright 2026
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "app_ui.h"

#include <stdio.h>

#include "lvgl.h"

#include "ui_i18n.h"
#include "ui_scope_view.h"
#include "ui_theme.h"

#define UI_SCREEN_WIDTH_PX  480
#define UI_SCREEN_HEIGHT_PX 320
#define UI_SCOPE_REFRESH_PERIOD_MS 16U

static lv_obj_t *s_title;
static lv_obj_t *s_sample_rate;
static lv_obj_t *s_status;
static lv_obj_t *s_measurement;
static lv_obj_t *s_mode_button;
static lv_obj_t *s_mode_button_label;
static lv_obj_t *s_language_button;
static lv_obj_t *s_language_button_label;
static ui_scope_mode_t s_mode = UI_SCOPE_MODE_WAVEFORM;
static bool s_core_ready;
static bool s_snapshot_valid;
static osc_snapshot_t s_snapshot;
static ui_text_id_t s_status_text_id = UI_TEXT_WAITING;
static uint32_t s_last_scope_refresh_tick;

static void ui_apply_font(lv_obj_t *object, const lv_font_t *font)
{
    lv_obj_set_style_text_font(object, font, LV_PART_MAIN);
}

static void ui_update_mode_text(void)
{
    const char *label = (UI_SCOPE_MODE_WAVEFORM == s_mode) ?
        ui_tr(UI_TEXT_SPECTRUM) : ui_tr(UI_TEXT_WAVEFORM);
    lv_label_set_text(s_mode_button_label, label);
}

static ui_text_id_t ui_current_status_text(void)
{
    if (!s_core_ready)
    {
        return UI_TEXT_WAITING;
    }
    if (s_snapshot_valid &&
        (0U != (s_snapshot.flags & OSC_FLAG_ADC_INIT_ERROR)))
    {
        return UI_TEXT_CORE_ERROR;
    }
    if (s_snapshot_valid &&
        (0U != (s_snapshot.flags & OSC_FLAG_INPUT_CLIPPED)))
    {
        return UI_TEXT_INPUT_CLIPPED;
    }
    if (s_snapshot_valid &&
        (0U != (s_snapshot.flags & OSC_FLAG_DMA_OVERRUN)))
    {
        return UI_TEXT_DMA_OVERRUN;
    }
    return s_snapshot_valid ? UI_TEXT_ACQUIRING : UI_TEXT_WAITING;
}

static void ui_update_status_text(void)
{
    ui_text_id_t status_text_id = ui_current_status_text();

    if (status_text_id != s_status_text_id)
    {
        s_status_text_id = status_text_id;
        lv_label_set_text(s_status, ui_tr(status_text_id));
    }
}

static void ui_update_language_text(void)
{
    ui_language_t language = ui_i18n_get_language();
    ui_text_id_t status_text_id = ui_current_status_text();
    const lv_font_t *title_font;
    const lv_font_t *body_font;

    ui_theme_set_language(language);
    title_font = ui_theme_title_font();
    body_font = ui_theme_body_font();
    ui_apply_font(s_title, title_font);
    ui_apply_font(s_sample_rate, body_font);
    ui_apply_font(s_status, body_font);
    ui_apply_font(s_measurement, body_font);
    ui_apply_font(s_mode_button_label, body_font);
    ui_apply_font(s_language_button_label, ui_theme_cjk_body_font());

    lv_label_set_text(s_title, ui_tr(UI_TEXT_TITLE));
    lv_label_set_text(s_sample_rate, ui_tr(UI_TEXT_SAMPLE_RATE));
    lv_label_set_text(
        s_language_button_label,
        (UI_LANGUAGE_ZH == language) ?
            ui_tr(UI_TEXT_LANGUAGE_ENGLISH) :
            ui_tr(UI_TEXT_LANGUAGE_CHINESE));
    ui_update_mode_text();
    lv_label_set_text(s_status, ui_tr(status_text_id));
    s_status_text_id = status_text_id;
}

static void ui_update_measurement(void)
{
    char text[96];
    uint32_t peak_hz;
    uint32_t peak_khz;
    uint32_t peak_fraction;
    uint32_t span;
    uint32_t span_mv;
    uint32_t span_volts;
    uint32_t span_fraction;

    if ((!s_core_ready) || (!s_snapshot_valid))
    {
        lv_label_set_text(s_measurement, "--");
        return;
    }

    span = (s_snapshot.maximum_raw >= s_snapshot.minimum_raw) ?
        ((uint32_t)s_snapshot.maximum_raw - s_snapshot.minimum_raw) : 0U;
    span_mv = (span * 3300U) / 65535U;
    span_volts = span_mv / 1000U;
    span_fraction = (span_mv % 1000U) / 10U;

    peak_hz = s_snapshot.peak_frequency_hz;
    peak_khz = peak_hz / 1000U;
    peak_fraction = (peak_hz % 1000U) / 10U;

    (void)snprintf(
        text,
        sizeof(text),
        ui_tr(UI_TEXT_MEASUREMENT_TEMPLATE),
        (unsigned int)span_volts,
        (unsigned int)span_fraction,
        (unsigned int)peak_khz,
        (unsigned int)peak_fraction);
    lv_label_set_text(s_measurement, text);
}

static void ui_language_event(lv_event_t *event)
{
    if (LV_EVENT_CLICKED == lv_event_get_code(event))
    {
        ui_language_t language = ui_i18n_get_language();
        ui_i18n_set_language(
            (UI_LANGUAGE_ZH == language) ?
                UI_LANGUAGE_EN : UI_LANGUAGE_ZH);
        ui_update_language_text();
        ui_update_measurement();
    }
}

static void ui_mode_event(lv_event_t *event)
{
    if (LV_EVENT_CLICKED == lv_event_get_code(event))
    {
        s_mode = (UI_SCOPE_MODE_WAVEFORM == s_mode) ?
            UI_SCOPE_MODE_SPECTRUM : UI_SCOPE_MODE_WAVEFORM;
        wd_scope_view_set_mode(s_mode);
        ui_update_mode_text();
        wd_scope_view_render(&s_snapshot, s_snapshot_valid);
    }
}

void app_ui_init(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_t *controls_panel;
    lv_obj_t *info_panel;

    ui_i18n_set_language(UI_LANGUAGE_ZH);
    ui_theme_init();
    ui_theme_set_language(UI_LANGUAGE_ZH);

    lv_obj_set_size(screen, UI_SCREEN_WIDTH_PX, UI_SCREEN_HEIGHT_PX);
    ui_style_apply_screen(screen);
    lv_obj_remove_flag(screen, LV_OBJ_FLAG_SCROLLABLE);

    s_title = lv_label_create(screen);
    lv_label_set_text(s_title, ui_tr(UI_TEXT_TITLE));
    ui_style_apply_text_primary(s_title);
    ui_apply_font(s_title, ui_theme_title_font());
    lv_obj_set_pos(s_title, 12, 4);

    s_sample_rate = lv_label_create(screen);
    lv_label_set_text(s_sample_rate, ui_tr(UI_TEXT_SAMPLE_RATE));
    ui_style_apply_text_muted(s_sample_rate);
    ui_apply_font(s_sample_rate, ui_theme_body_font());
    lv_obj_set_pos(s_sample_rate, 236, 15);

    s_language_button = lv_button_create(screen);
    ui_style_apply_button_secondary(s_language_button);
    lv_obj_set_size(s_language_button, 50, 36);
    lv_obj_set_pos(s_language_button, 420, 4);
    lv_obj_set_ext_click_area(s_language_button, 6);
    lv_obj_add_event_cb(
        s_language_button, ui_language_event, LV_EVENT_CLICKED, NULL);

    s_language_button_label = lv_label_create(s_language_button);
    lv_label_set_text(
        s_language_button_label, ui_tr(UI_TEXT_LANGUAGE_ENGLISH));
    ui_apply_font(s_language_button_label, ui_theme_body_font());
    lv_obj_center(s_language_button_label);

    wd_scope_view_init(screen);

    controls_panel = lv_obj_create(screen);
    ui_style_apply_control_bar(controls_panel);
    lv_obj_set_pos(controls_panel, 8, 256);
    lv_obj_set_size(controls_panel, 464, 58);
    lv_obj_set_flex_flow(controls_panel, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(
        controls_panel,
        LV_FLEX_ALIGN_START,
        LV_FLEX_ALIGN_CENTER,
        LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(controls_panel, LV_OBJ_FLAG_SCROLLABLE);

    info_panel = lv_obj_create(controls_panel);
    ui_style_apply_info_panel(info_panel);
    lv_obj_set_height(info_panel, 48);
    lv_obj_set_flex_grow(info_panel, 1);
    lv_obj_remove_flag(info_panel, LV_OBJ_FLAG_SCROLLABLE);

    s_status = lv_label_create(info_panel);
    lv_label_set_text(s_status, ui_tr(UI_TEXT_WAITING));
    ui_style_apply_text_muted(s_status);
    ui_apply_font(s_status, ui_theme_body_font());
    lv_obj_set_pos(s_status, 8, 3);

    s_measurement = lv_label_create(info_panel);
    lv_label_set_text(s_measurement, "--");
    ui_style_apply_text_primary(s_measurement);
    ui_apply_font(s_measurement, ui_theme_body_font());
    lv_obj_set_pos(s_measurement, 8, 24);

    s_mode_button = lv_button_create(controls_panel);
    ui_style_apply_button(s_mode_button);
    lv_obj_set_size(s_mode_button, 94, 48);
    lv_obj_add_event_cb(
        s_mode_button, ui_mode_event, LV_EVENT_CLICKED, NULL);

    s_mode_button_label = lv_label_create(s_mode_button);
    lv_label_set_text(
        s_mode_button_label, ui_tr(UI_TEXT_SPECTRUM));
    ui_apply_font(s_mode_button_label, ui_theme_body_font());
    lv_obj_center(s_mode_button_label);

    wd_scope_view_set_mode(s_mode);
    wd_scope_view_render(&s_snapshot, false);
    s_last_scope_refresh_tick = lv_tick_get();
}

void app_ui_set_core_ready(bool ready)
{
    s_core_ready = ready;
    ui_update_language_text();
    ui_update_measurement();
}

void app_ui_update(const osc_snapshot_t *snapshot, bool valid)
{
    uint32_t previous_buffers = s_snapshot.completed_buffers;

    s_snapshot_valid = valid;
    if (valid)
    {
        s_snapshot = *snapshot;
    }

    ui_update_status_text();
    ui_update_measurement();
    if (valid && (s_snapshot.completed_buffers != previous_buffers))
    {
        uint32_t now = lv_tick_get();

        if ((uint32_t)(now - s_last_scope_refresh_tick) >=
            UI_SCOPE_REFRESH_PERIOD_MS)
        {
            wd_scope_view_render(&s_snapshot, true);
            s_last_scope_refresh_tick = now;
        }
    }
}
