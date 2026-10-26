/*
 * Copyright 2026
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ui_theme.h"

LV_FONT_DECLARE(osc_font_zh_16);
LV_FONT_DECLARE(osc_font_zh_24);

enum
{
    UI_SPACE_1 = 4,
    UI_SPACE_2 = 8,
    UI_SPACE_3 = 12,
    UI_RADIUS_PANEL = 12,
    UI_RADIUS_BUTTON = 8,
};

static lv_style_t style_screen;
static lv_style_t style_panel;
static lv_style_t style_control_bar;
static lv_style_t style_info_panel;
static lv_style_t style_text_primary;
static lv_style_t style_text_muted;
static lv_style_t style_button;
static lv_style_t style_button_pressed;
static lv_style_t style_button_secondary;
static lv_style_t style_button_secondary_pressed;
static lv_style_t style_scope_plot;
static lv_style_t style_scope_axis;
static lv_style_t style_scope_tick;
static lv_style_t style_scope_axis_label;
static lv_style_t style_scope_line;
static lv_style_transition_dsc_t style_button_transition;
static ui_language_t s_language = UI_LANGUAGE_ZH;

static const lv_style_prop_t button_transition_props[] = {
    LV_STYLE_BG_COLOR,
    LV_STYLE_BORDER_COLOR,
    LV_STYLE_TEXT_COLOR,
    0,
};

static lv_color32_t color32(uint8_t red, uint8_t green, uint8_t blue)
{
    lv_color32_t color = {
        .blue = blue,
        .green = green,
        .red = red,
        .alpha = 0xFF,
    };
    return color;
}

void ui_theme_init(void)
{
    lv_style_init(&style_screen);
    lv_style_set_bg_color(&style_screen, lv_color_hex(0x07131E));
    lv_style_set_text_color(&style_screen, lv_color_hex(0xDDF6FF));

    lv_style_init(&style_panel);
    lv_style_set_bg_color(&style_panel, lv_color_hex(0x102535));
    lv_style_set_bg_opa(&style_panel, LV_OPA_COVER);
    lv_style_set_border_width(&style_panel, 1);
    lv_style_set_border_color(&style_panel, lv_color_hex(0x21475B));
    lv_style_set_radius(&style_panel, UI_RADIUS_PANEL);
    lv_style_set_pad_all(&style_panel, UI_SPACE_2);

    lv_style_init(&style_control_bar);
    lv_style_set_bg_color(&style_control_bar, lv_color_hex(0x102535));
    lv_style_set_bg_opa(&style_control_bar, LV_OPA_COVER);
    lv_style_set_border_width(&style_control_bar, 1);
    lv_style_set_border_color(&style_control_bar, lv_color_hex(0x21475B));
    lv_style_set_radius(&style_control_bar, UI_RADIUS_PANEL);
    lv_style_set_pad_all(&style_control_bar, UI_SPACE_1);
    lv_style_set_pad_column(&style_control_bar, UI_SPACE_2);

    lv_style_init(&style_info_panel);
    lv_style_set_bg_color(&style_info_panel, lv_color_hex(0x0B1D29));
    lv_style_set_bg_opa(&style_info_panel, LV_OPA_COVER);
    lv_style_set_border_width(&style_info_panel, 0);
    lv_style_set_radius(&style_info_panel, UI_RADIUS_BUTTON);
    lv_style_set_pad_all(&style_info_panel, 0);

    lv_style_init(&style_text_primary);
    lv_style_set_text_color(&style_text_primary, lv_color_hex(0xDDF6FF));

    lv_style_init(&style_text_muted);
    lv_style_set_text_color(&style_text_muted, lv_color_hex(0x7FA4B5));

    lv_style_init(&style_button);
    lv_style_set_bg_color(&style_button, lv_color_hex(0x24D6C4));
    lv_style_set_bg_opa(&style_button, LV_OPA_COVER);
    lv_style_set_border_width(&style_button, 0);
    lv_style_set_radius(&style_button, UI_RADIUS_BUTTON);
    lv_style_set_text_color(&style_button, lv_color_hex(0x031416));
    lv_style_set_pad_hor(&style_button, UI_SPACE_2);
    lv_style_set_pad_ver(&style_button, UI_SPACE_1);

    lv_style_init(&style_button_pressed);
    lv_style_set_bg_color(&style_button_pressed, lv_color_hex(0x15988C));
    lv_style_set_text_color(&style_button_pressed, lv_color_hex(0xFFFFFF));

    lv_style_init(&style_button_secondary);
    lv_style_set_bg_color(&style_button_secondary, lv_color_hex(0x173446));
    lv_style_set_bg_opa(&style_button_secondary, LV_OPA_COVER);
    lv_style_set_border_width(&style_button_secondary, 1);
    lv_style_set_border_color(&style_button_secondary, lv_color_hex(0x3B6C80));
    lv_style_set_radius(&style_button_secondary, UI_RADIUS_BUTTON);
    lv_style_set_text_color(&style_button_secondary, lv_color_hex(0xDDF6FF));
    lv_style_set_pad_hor(&style_button_secondary, UI_SPACE_2);
    lv_style_set_pad_ver(&style_button_secondary, UI_SPACE_1);

    lv_style_init(&style_button_secondary_pressed);
    lv_style_set_bg_color(
        &style_button_secondary_pressed, lv_color_hex(0x214D61));
    lv_style_set_border_color(
        &style_button_secondary_pressed, lv_color_hex(0x24D6C4));

    lv_style_init(&style_scope_plot);
    lv_style_set_bg_color(&style_scope_plot, lv_color_hex(0x07131E));
    lv_style_set_bg_opa(&style_scope_plot, LV_OPA_COVER);
    lv_style_set_border_width(&style_scope_plot, 1);
    lv_style_set_border_color(&style_scope_plot, lv_color_hex(0x21475B));
    lv_style_set_radius(&style_scope_plot, UI_SPACE_1);
    lv_style_set_pad_all(&style_scope_plot, 0);

    lv_style_init(&style_scope_axis);
    lv_style_set_bg_color(&style_scope_axis, lv_color_hex(0x21475B));
    lv_style_set_bg_opa(&style_scope_axis, LV_OPA_COVER);
    lv_style_set_border_width(&style_scope_axis, 0);
    lv_style_set_radius(&style_scope_axis, 0);

    lv_style_init(&style_scope_tick);
    lv_style_set_bg_color(&style_scope_tick, lv_color_hex(0x5B8294));
    lv_style_set_bg_opa(&style_scope_tick, LV_OPA_COVER);
    lv_style_set_border_width(&style_scope_tick, 0);
    lv_style_set_radius(&style_scope_tick, 1);

    lv_style_init(&style_scope_axis_label);
    lv_style_set_bg_opa(&style_scope_axis_label, LV_OPA_TRANSP);
    lv_style_set_text_color(&style_scope_axis_label, lv_color_hex(0x7FA4B5));
    lv_style_set_pad_all(&style_scope_axis_label, 0);

    lv_style_init(&style_scope_line);
    lv_style_set_line_color(&style_scope_line, lv_color_hex(0x24D6C4));
    lv_style_set_line_width(&style_scope_line, 2);
    lv_style_set_line_rounded(&style_scope_line, true);

    lv_style_transition_dsc_init(
        &style_button_transition,
        button_transition_props,
        lv_anim_path_ease_out,
        120,
        0,
        NULL);
    lv_style_set_transition(
        &style_button_pressed, &style_button_transition);
    lv_style_set_transition(
        &style_button_secondary_pressed, &style_button_transition);
}

void ui_theme_set_language(ui_language_t language)
{
    s_language = (language < UI_LANGUAGE_COUNT) ?
        language : UI_LANGUAGE_ZH;
}

const lv_font_t *ui_theme_title_font(void)
{
    return (UI_LANGUAGE_ZH == s_language) ?
        &osc_font_zh_24 : &lv_font_montserrat_24;
}

const lv_font_t *ui_theme_body_font(void)
{
    return (UI_LANGUAGE_ZH == s_language) ?
        &osc_font_zh_16 : &lv_font_montserrat_16;
}

const lv_font_t *ui_theme_cjk_body_font(void)
{
    return &osc_font_zh_16;
}

const lv_font_t *ui_theme_axis_font(void)
{
    return &lv_font_montserrat_14;
}

void ui_style_apply_screen(lv_obj_t *object)
{
    lv_obj_add_style(object, &style_screen, LV_PART_MAIN);
}

void ui_style_apply_panel(lv_obj_t *object)
{
    lv_obj_add_style(object, &style_panel, LV_PART_MAIN);
}

void ui_style_apply_control_bar(lv_obj_t *object)
{
    lv_obj_add_style(object, &style_control_bar, LV_PART_MAIN);
}

void ui_style_apply_info_panel(lv_obj_t *object)
{
    lv_obj_add_style(object, &style_info_panel, LV_PART_MAIN);
}

void ui_style_apply_button(lv_obj_t *object)
{
    lv_obj_add_style(object, &style_button, LV_PART_MAIN);
    lv_obj_add_style(
        object, &style_button_pressed, LV_PART_MAIN | LV_STATE_PRESSED);
}

void ui_style_apply_button_secondary(lv_obj_t *object)
{
    lv_obj_add_style(object, &style_button_secondary, LV_PART_MAIN);
    lv_obj_add_style(
        object,
        &style_button_secondary_pressed,
        LV_PART_MAIN | LV_STATE_PRESSED);
}

void ui_style_apply_text_primary(lv_obj_t *object)
{
    lv_obj_add_style(object, &style_text_primary, LV_PART_MAIN);
}

void ui_style_apply_text_muted(lv_obj_t *object)
{
    lv_obj_add_style(object, &style_text_muted, LV_PART_MAIN);
}

void ui_style_apply_scope_plot(lv_obj_t *object)
{
    lv_obj_add_style(object, &style_scope_plot, LV_PART_MAIN);
}

void ui_style_apply_scope_axis(lv_obj_t *object)
{
    lv_obj_add_style(object, &style_scope_axis, LV_PART_MAIN);
}

void ui_style_apply_scope_tick(lv_obj_t *object)
{
    lv_obj_add_style(object, &style_scope_tick, LV_PART_MAIN);
}

void ui_style_apply_scope_axis_label(lv_obj_t *object)
{
    lv_obj_add_style(object, &style_scope_axis_label, LV_PART_MAIN);
}

void ui_style_apply_scope_line(lv_obj_t *object)
{
    lv_obj_add_style(object, &style_scope_line, LV_PART_MAIN);
}

lv_color32_t ui_theme_scope_palette(uint8_t index)
{
    return (0U == index) ?
        color32(0x07, 0x13, 0x1E) :
        color32(0x24, 0xD6, 0xC4);
}
