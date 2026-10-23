/*
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UI_THEME_H
#define UI_THEME_H

#include "lvgl.h"

#define UI_COLOR_BG lv_color_hex(0xF2F5F7)
#define UI_COLOR_SURFACE lv_color_hex(0xFFFFFF)
#define UI_COLOR_INK lv_color_hex(0x17212B)
#define UI_COLOR_MUTED lv_color_hex(0x637383)
#define UI_COLOR_ACCENT lv_color_hex(0x087A75)
#define UI_COLOR_ACCENT_DARK lv_color_hex(0x05635F)
#define UI_COLOR_ACCENT_SOFT lv_color_hex(0xDCEFED)
#define UI_COLOR_BORDER lv_color_hex(0xCED8DF)
#define UI_COLOR_DANGER lv_color_hex(0xC44343)
#define UI_COLOR_DANGER_SOFT lv_color_hex(0xF8E3E3)

#define UI_FONT_TITLE (&lv_font_montserrat_24)
#define UI_FONT_BODY (&lv_font_montserrat_16)

#define UI_RADIUS_CARD 12
#define UI_RADIUS_CONTROL 8
#define UI_TRANSITION_MS 110

typedef enum {
    UI_LABEL_TITLE,
    UI_LABEL_BODY,
    UI_LABEL_MUTED,
    UI_LABEL_ACCENT,
    UI_LABEL_BUTTON
} ui_label_role_t;

void ui_theme_init(void);
void ui_style_apply_screen(lv_obj_t *obj);
void ui_style_apply_transparent(lv_obj_t *obj);
void ui_style_apply_card(lv_obj_t *obj);
void ui_style_apply_card_flat(lv_obj_t *obj);
void ui_style_apply_settings_card(lv_obj_t *obj);
void ui_style_apply_canvas_frame(lv_obj_t *obj);
void ui_style_apply_canvas(lv_obj_t *obj);
void ui_style_apply_label(lv_obj_t *label, ui_label_role_t role);
void ui_style_apply_button_primary(lv_obj_t *button);
void ui_style_apply_button_danger(lv_obj_t *button);
void ui_style_apply_option_button(lv_obj_t *button);
void ui_style_apply_menu_item(lv_obj_t *button);
void ui_style_apply_menu_panel(lv_obj_t *obj);
void ui_style_apply_info_bar(lv_obj_t *obj);
void ui_style_apply_switch_track(lv_obj_t *obj);
void ui_style_apply_switch_knob(lv_obj_t *obj);
void ui_style_apply_badge(lv_obj_t *obj);
void ui_style_apply_accuracy_target(lv_obj_t *obj);
void ui_style_apply_accuracy_mark(lv_obj_t *obj);

#endif
