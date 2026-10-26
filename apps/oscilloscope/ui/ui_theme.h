/*
 * Copyright 2026
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UI_THEME_H_
#define UI_THEME_H_

#include "lvgl.h"

#include "ui_i18n.h"

void ui_theme_init(void);
void ui_theme_set_language(ui_language_t language);

const lv_font_t *ui_theme_title_font(void);
const lv_font_t *ui_theme_body_font(void);
const lv_font_t *ui_theme_cjk_body_font(void);
const lv_font_t *ui_theme_axis_font(void);

void ui_style_apply_screen(lv_obj_t *object);
void ui_style_apply_panel(lv_obj_t *object);
void ui_style_apply_control_bar(lv_obj_t *object);
void ui_style_apply_info_panel(lv_obj_t *object);
void ui_style_apply_button(lv_obj_t *object);
void ui_style_apply_button_secondary(lv_obj_t *object);
void ui_style_apply_text_primary(lv_obj_t *object);
void ui_style_apply_text_muted(lv_obj_t *object);
void ui_style_apply_scope_plot(lv_obj_t *object);
void ui_style_apply_scope_axis(lv_obj_t *object);
void ui_style_apply_scope_tick(lv_obj_t *object);
void ui_style_apply_scope_axis_label(lv_obj_t *object);
void ui_style_apply_scope_line(lv_obj_t *object);

lv_color32_t ui_theme_scope_palette(uint8_t index);

#endif /* UI_THEME_H_ */
