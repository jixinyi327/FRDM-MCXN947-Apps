/*
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ui_theme.h"

static lv_style_t style_screen;
static lv_style_t style_transparent;
static lv_style_t style_card;
static lv_style_t style_card_flat;
static lv_style_t style_settings_card;
static lv_style_t style_canvas_frame;
static lv_style_t style_canvas;
static lv_style_t style_label_title;
static lv_style_t style_label_body;
static lv_style_t style_label_muted;
static lv_style_t style_label_accent;
static lv_style_t style_label_button;
static lv_style_t style_button_primary;
static lv_style_t style_button_primary_pressed;
static lv_style_t style_button_primary_disabled;
static lv_style_t style_button_danger;
static lv_style_t style_button_danger_pressed;
static lv_style_t style_option_button;
static lv_style_t style_option_checked;
static lv_style_t style_option_checked_pressed;
static lv_style_t style_menu_item;
static lv_style_t style_menu_item_checked;
static lv_style_t style_menu_panel;
static lv_style_t style_info_bar;
static lv_style_t style_switch_track;
static lv_style_t style_switch_track_checked;
static lv_style_t style_switch_knob;
static lv_style_t style_badge;
static lv_style_t style_accuracy_target;
static lv_style_t style_accuracy_mark;
static lv_style_t style_button_focused;
static lv_style_transition_dsc_t style_transition_in;
static lv_style_transition_dsc_t style_transition_out;
static bool s_theme_initialized;

static const lv_style_prop_t s_state_transition_props[] = {
    LV_STYLE_BG_COLOR,
    LV_STYLE_BG_OPA,
    LV_STYLE_BORDER_COLOR,
    LV_STYLE_BORDER_WIDTH,
    LV_STYLE_TEXT_COLOR,
    0,
};

static void init_common_button_style(lv_style_t *style)
{
    lv_style_set_radius(style, UI_RADIUS_CONTROL);
    lv_style_set_pad_all(style, 0);
    lv_style_set_text_font(style, UI_FONT_BODY);
    lv_style_set_transition(style, &style_transition_in);
}

void ui_theme_init(void)
{
    if (s_theme_initialized)
    {
        return;
    }

    lv_style_transition_dsc_init(&style_transition_in,
                                 s_state_transition_props,
                                 lv_anim_path_ease_out,
                                 UI_TRANSITION_MS,
                                 0,
                                 NULL);
    lv_style_transition_dsc_init(&style_transition_out,
                                 s_state_transition_props,
                                 lv_anim_path_ease_out,
                                 UI_TRANSITION_MS,
                                 40,
                                 NULL);

    lv_style_init(&style_screen);
    lv_style_set_bg_color(&style_screen, UI_COLOR_BG);
    lv_style_set_bg_opa(&style_screen, LV_OPA_COVER);
    lv_style_set_border_width(&style_screen, 0);
    lv_style_set_pad_all(&style_screen, 0);
    lv_style_set_radius(&style_screen, 0);

    lv_style_init(&style_transparent);
    lv_style_set_bg_opa(&style_transparent, LV_OPA_TRANSP);
    lv_style_set_border_width(&style_transparent, 0);
    lv_style_set_pad_all(&style_transparent, 0);
    lv_style_set_radius(&style_transparent, 0);

    lv_style_init(&style_card);
    lv_style_set_bg_color(&style_card, UI_COLOR_SURFACE);
    lv_style_set_bg_opa(&style_card, LV_OPA_COVER);
    lv_style_set_border_color(&style_card, UI_COLOR_BORDER);
    lv_style_set_border_width(&style_card, 1);
    lv_style_set_radius(&style_card, UI_RADIUS_CARD);
    lv_style_set_pad_all(&style_card, 12);

    lv_style_init(&style_card_flat);
    lv_style_set_bg_color(&style_card_flat, UI_COLOR_SURFACE);
    lv_style_set_bg_opa(&style_card_flat, LV_OPA_COVER);
    lv_style_set_border_color(&style_card_flat, UI_COLOR_BORDER);
    lv_style_set_border_width(&style_card_flat, 1);
    lv_style_set_radius(&style_card_flat, UI_RADIUS_CARD);
    lv_style_set_pad_all(&style_card_flat, 0);

    lv_style_init(&style_settings_card);
    lv_style_set_bg_color(&style_settings_card, UI_COLOR_SURFACE);
    lv_style_set_bg_opa(&style_settings_card, LV_OPA_COVER);
    lv_style_set_border_color(&style_settings_card, UI_COLOR_BORDER);
    lv_style_set_border_width(&style_settings_card, 1);
    lv_style_set_radius(&style_settings_card, UI_RADIUS_CARD);
    lv_style_set_pad_hor(&style_settings_card, 12);
    lv_style_set_pad_ver(&style_settings_card, 10);

    lv_style_init(&style_canvas_frame);
    lv_style_set_bg_color(&style_canvas_frame, UI_COLOR_SURFACE);
    lv_style_set_bg_opa(&style_canvas_frame, LV_OPA_COVER);
    lv_style_set_border_color(&style_canvas_frame, UI_COLOR_BORDER);
    lv_style_set_border_width(&style_canvas_frame, 1);
    lv_style_set_radius(&style_canvas_frame, UI_RADIUS_CONTROL);
    lv_style_set_pad_all(&style_canvas_frame, 0);

    lv_style_init(&style_canvas);
    lv_style_set_bg_opa(&style_canvas, LV_OPA_TRANSP);
    lv_style_set_border_width(&style_canvas, 0);
    lv_style_set_radius(&style_canvas, 0);
    lv_style_set_pad_all(&style_canvas, 0);

    lv_style_init(&style_label_title);
    lv_style_set_text_font(&style_label_title, UI_FONT_TITLE);
    lv_style_set_text_color(&style_label_title, UI_COLOR_INK);

    lv_style_init(&style_label_body);
    lv_style_set_text_font(&style_label_body, UI_FONT_BODY);
    lv_style_set_text_color(&style_label_body, UI_COLOR_INK);

    lv_style_init(&style_label_muted);
    lv_style_set_text_font(&style_label_muted, UI_FONT_BODY);
    lv_style_set_text_color(&style_label_muted, UI_COLOR_MUTED);

    lv_style_init(&style_label_accent);
    lv_style_set_text_font(&style_label_accent, UI_FONT_BODY);
    lv_style_set_text_color(&style_label_accent, UI_COLOR_ACCENT);

    lv_style_init(&style_label_button);
    lv_style_set_text_font(&style_label_button, UI_FONT_BODY);

    lv_style_init(&style_button_primary);
    init_common_button_style(&style_button_primary);
    lv_style_set_bg_color(&style_button_primary, UI_COLOR_ACCENT);
    lv_style_set_bg_opa(&style_button_primary, LV_OPA_COVER);
    lv_style_set_border_width(&style_button_primary, 0);
    lv_style_set_text_color(&style_button_primary, lv_color_white());

    lv_style_init(&style_button_primary_pressed);
    lv_style_set_bg_color(&style_button_primary_pressed, UI_COLOR_ACCENT_DARK);
    lv_style_set_transition(&style_button_primary_pressed, &style_transition_out);

    lv_style_init(&style_button_primary_disabled);
    lv_style_set_bg_color(&style_button_primary_disabled, UI_COLOR_BORDER);
    lv_style_set_text_color(&style_button_primary_disabled, UI_COLOR_MUTED);
    lv_style_set_transition(&style_button_primary_disabled, &style_transition_out);

    lv_style_init(&style_button_danger);
    init_common_button_style(&style_button_danger);
    lv_style_set_bg_color(&style_button_danger, UI_COLOR_SURFACE);
    lv_style_set_bg_opa(&style_button_danger, LV_OPA_COVER);
    lv_style_set_border_color(&style_button_danger, UI_COLOR_DANGER);
    lv_style_set_border_width(&style_button_danger, 1);
    lv_style_set_text_color(&style_button_danger, UI_COLOR_DANGER);

    lv_style_init(&style_button_danger_pressed);
    lv_style_set_bg_color(&style_button_danger_pressed, UI_COLOR_DANGER_SOFT);
    lv_style_set_transition(&style_button_danger_pressed, &style_transition_out);

    lv_style_init(&style_option_button);
    init_common_button_style(&style_option_button);
    lv_style_set_bg_color(&style_option_button, UI_COLOR_SURFACE);
    lv_style_set_bg_opa(&style_option_button, LV_OPA_COVER);
    lv_style_set_border_color(&style_option_button, UI_COLOR_BORDER);
    lv_style_set_border_width(&style_option_button, 1);
    lv_style_set_text_color(&style_option_button, UI_COLOR_INK);

    lv_style_init(&style_option_checked);
    lv_style_set_bg_color(&style_option_checked, UI_COLOR_ACCENT);
    lv_style_set_border_color(&style_option_checked, UI_COLOR_ACCENT);
    lv_style_set_border_width(&style_option_checked, 0);
    lv_style_set_text_color(&style_option_checked, lv_color_white());

    lv_style_init(&style_option_checked_pressed);
    lv_style_set_bg_color(&style_option_checked_pressed, UI_COLOR_ACCENT_DARK);
    lv_style_set_transition(&style_option_checked_pressed, &style_transition_out);

    lv_style_init(&style_menu_item);
    lv_style_set_bg_color(&style_menu_item, UI_COLOR_SURFACE);
    lv_style_set_bg_opa(&style_menu_item, LV_OPA_COVER);
    lv_style_set_border_width(&style_menu_item, 0);
    lv_style_set_radius(&style_menu_item, UI_RADIUS_CONTROL);
    lv_style_set_pad_all(&style_menu_item, 0);
    lv_style_set_text_font(&style_menu_item, UI_FONT_BODY);
    lv_style_set_text_color(&style_menu_item, UI_COLOR_INK);

    lv_style_init(&style_menu_item_checked);
    lv_style_set_bg_color(&style_menu_item_checked, UI_COLOR_ACCENT_SOFT);

    lv_style_init(&style_menu_panel);
    lv_style_set_bg_color(&style_menu_panel, UI_COLOR_SURFACE);
    lv_style_set_bg_opa(&style_menu_panel, LV_OPA_COVER);
    lv_style_set_border_color(&style_menu_panel, UI_COLOR_BORDER);
    lv_style_set_border_width(&style_menu_panel, 1);
    lv_style_set_radius(&style_menu_panel, UI_RADIUS_CONTROL);
    lv_style_set_pad_all(&style_menu_panel, 8);
    lv_style_set_pad_row(&style_menu_panel, 6);

    lv_style_init(&style_info_bar);
    lv_style_set_bg_color(&style_info_bar, UI_COLOR_SURFACE);
    lv_style_set_bg_opa(&style_info_bar, LV_OPA_COVER);
    lv_style_set_border_color(&style_info_bar, UI_COLOR_BORDER);
    lv_style_set_border_width(&style_info_bar, 1);
    lv_style_set_radius(&style_info_bar, UI_RADIUS_CONTROL);
    lv_style_set_pad_all(&style_info_bar, 0);

    lv_style_init(&style_switch_track);
    lv_style_set_bg_color(&style_switch_track, UI_COLOR_BORDER);
    lv_style_set_bg_opa(&style_switch_track, LV_OPA_COVER);
    lv_style_set_border_width(&style_switch_track, 0);
    lv_style_set_radius(&style_switch_track, LV_RADIUS_CIRCLE);
    lv_style_set_pad_all(&style_switch_track, 0);

    lv_style_init(&style_switch_track_checked);
    lv_style_set_bg_color(&style_switch_track_checked, UI_COLOR_ACCENT);

    lv_style_init(&style_switch_knob);
    lv_style_set_bg_color(&style_switch_knob, lv_color_white());
    lv_style_set_bg_opa(&style_switch_knob, LV_OPA_COVER);
    lv_style_set_border_width(&style_switch_knob, 0);
    lv_style_set_radius(&style_switch_knob, LV_RADIUS_CIRCLE);
    lv_style_set_pad_all(&style_switch_knob, 0);

    lv_style_init(&style_badge);
    lv_style_set_bg_color(&style_badge, UI_COLOR_ACCENT_SOFT);
    lv_style_set_bg_opa(&style_badge, LV_OPA_COVER);
    lv_style_set_border_width(&style_badge, 0);
    lv_style_set_radius(&style_badge, LV_RADIUS_CIRCLE);
    lv_style_set_pad_all(&style_badge, 0);

    lv_style_init(&style_accuracy_target);
    lv_style_set_bg_opa(&style_accuracy_target, LV_OPA_TRANSP);
    lv_style_set_border_color(&style_accuracy_target, UI_COLOR_ACCENT);
    lv_style_set_border_width(&style_accuracy_target, 3);
    lv_style_set_radius(&style_accuracy_target, LV_RADIUS_CIRCLE);
    lv_style_set_pad_all(&style_accuracy_target, 0);

    lv_style_init(&style_accuracy_mark);
    lv_style_set_bg_color(&style_accuracy_mark, UI_COLOR_ACCENT);
    lv_style_set_bg_opa(&style_accuracy_mark, LV_OPA_COVER);
    lv_style_set_border_width(&style_accuracy_mark, 0);
    lv_style_set_radius(&style_accuracy_mark, LV_RADIUS_CIRCLE);
    lv_style_set_pad_all(&style_accuracy_mark, 0);

    lv_style_init(&style_button_focused);
    lv_style_set_outline_color(&style_button_focused, UI_COLOR_ACCENT);
    lv_style_set_outline_width(&style_button_focused, 2);
    lv_style_set_outline_pad(&style_button_focused, 2);

    s_theme_initialized = true;
}

void ui_style_apply_screen(lv_obj_t *obj)
{
    lv_obj_add_style(obj, &style_screen, LV_PART_MAIN);
}

void ui_style_apply_transparent(lv_obj_t *obj)
{
    lv_obj_add_style(obj, &style_transparent, LV_PART_MAIN);
}

void ui_style_apply_card(lv_obj_t *obj)
{
    lv_obj_add_style(obj, &style_card, LV_PART_MAIN);
}

void ui_style_apply_card_flat(lv_obj_t *obj)
{
    lv_obj_add_style(obj, &style_card_flat, LV_PART_MAIN);
}

void ui_style_apply_settings_card(lv_obj_t *obj)
{
    lv_obj_add_style(obj, &style_settings_card, LV_PART_MAIN);
}

void ui_style_apply_canvas_frame(lv_obj_t *obj)
{
    lv_obj_add_style(obj, &style_canvas_frame, LV_PART_MAIN);
}

void ui_style_apply_canvas(lv_obj_t *obj)
{
    lv_obj_add_style(obj, &style_canvas, LV_PART_MAIN);
}

void ui_style_apply_label(lv_obj_t *label, ui_label_role_t role)
{
    switch (role)
    {
        case UI_LABEL_TITLE:
            lv_obj_add_style(label, &style_label_title, LV_PART_MAIN);
            break;
        case UI_LABEL_BODY:
            lv_obj_add_style(label, &style_label_body, LV_PART_MAIN);
            break;
        case UI_LABEL_MUTED:
            lv_obj_add_style(label, &style_label_muted, LV_PART_MAIN);
            break;
        case UI_LABEL_ACCENT:
            lv_obj_add_style(label, &style_label_accent, LV_PART_MAIN);
            break;
        case UI_LABEL_BUTTON:
            lv_obj_add_style(label, &style_label_button, LV_PART_MAIN);
            break;
        default:
            break;
    }
}

void ui_style_apply_button_primary(lv_obj_t *button)
{
    lv_obj_add_style(button, &style_button_primary, LV_PART_MAIN);
    lv_obj_add_style(button, &style_button_primary_pressed,
                     LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_style(button, &style_button_primary_disabled,
                     LV_PART_MAIN | LV_STATE_DISABLED);
    lv_obj_add_style(button, &style_button_focused,
                     LV_PART_MAIN | LV_STATE_FOCUSED);
}

void ui_style_apply_button_danger(lv_obj_t *button)
{
    lv_obj_add_style(button, &style_button_danger, LV_PART_MAIN);
    lv_obj_add_style(button, &style_button_danger_pressed,
                     LV_PART_MAIN | LV_STATE_PRESSED);
    lv_obj_add_style(button, &style_button_primary_disabled,
                     LV_PART_MAIN | LV_STATE_DISABLED);
    lv_obj_add_style(button, &style_button_focused,
                     LV_PART_MAIN | LV_STATE_FOCUSED);
}

void ui_style_apply_option_button(lv_obj_t *button)
{
    lv_obj_add_style(button, &style_option_button, LV_PART_MAIN);
    lv_obj_add_style(button, &style_option_checked,
                     LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_add_style(button, &style_option_checked_pressed,
                     LV_PART_MAIN | LV_STATE_CHECKED | LV_STATE_PRESSED);
    lv_obj_add_style(button, &style_button_focused,
                     LV_PART_MAIN | LV_STATE_FOCUSED);
}

void ui_style_apply_menu_item(lv_obj_t *button)
{
    lv_obj_add_style(button, &style_menu_item, LV_PART_MAIN);
    lv_obj_add_style(button, &style_menu_item_checked,
                     LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_add_style(button, &style_button_focused,
                     LV_PART_MAIN | LV_STATE_FOCUSED);
}

void ui_style_apply_menu_panel(lv_obj_t *obj)
{
    lv_obj_add_style(obj, &style_menu_panel, LV_PART_MAIN);
}

void ui_style_apply_info_bar(lv_obj_t *obj)
{
    lv_obj_add_style(obj, &style_info_bar, LV_PART_MAIN);
}

void ui_style_apply_switch_track(lv_obj_t *obj)
{
    lv_obj_add_style(obj, &style_switch_track, LV_PART_MAIN);
    lv_obj_add_style(obj, &style_switch_track_checked,
                     LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_add_style(obj, &style_button_focused,
                     LV_PART_MAIN | LV_STATE_FOCUSED);
}

void ui_style_apply_switch_knob(lv_obj_t *obj)
{
    lv_obj_add_style(obj, &style_switch_knob, LV_PART_MAIN);
}

void ui_style_apply_badge(lv_obj_t *obj)
{
    lv_obj_add_style(obj, &style_badge, LV_PART_MAIN);
}

void ui_style_apply_accuracy_target(lv_obj_t *obj)
{
    lv_obj_add_style(obj, &style_accuracy_target, LV_PART_MAIN);
}

void ui_style_apply_accuracy_mark(lv_obj_t *obj)
{
    lv_obj_add_style(obj, &style_accuracy_mark, LV_PART_MAIN);
}
