/*
 * Shared interaction and layout primitives for the touchscreen tester.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UI_COMMON_H
#define UI_COMMON_H

#include "lvgl.h"

#define UI_SCREEN_WIDTH 480
#define UI_SCREEN_HEIGHT 320
#define UI_HEADER_HEIGHT 52
#define UI_CONTENT_LEFT 20
#define UI_CONTENT_TOP 54
#define UI_CONTENT_WIDTH 440
#define UI_CONTENT_HEIGHT 184

#define UI_FOOTER_MARGIN 12
#define UI_FOOTER_HEIGHT 36
#define UI_FOOTER_BUTTON_WIDTH 72
#define UI_FOOTER_GAP 8
#define UI_FOOTER_TOP (UI_SCREEN_HEIGHT - UI_FOOTER_MARGIN - UI_FOOTER_HEIGHT)

#define UI_COLOR_BG lv_color_hex(0xF2F5F7)
#define UI_COLOR_SURFACE lv_color_hex(0xFFFFFF)
#define UI_COLOR_INK lv_color_hex(0x17212B)
#define UI_COLOR_MUTED lv_color_hex(0x637383)
#define UI_COLOR_ACCENT lv_color_hex(0x087A75)
#define UI_COLOR_ACCENT_SOFT lv_color_hex(0xDCEFED)
#define UI_COLOR_BORDER lv_color_hex(0xCED8DF)
#define UI_COLOR_DANGER lv_color_hex(0xC44343)

typedef enum {
    UI_PAGE_FREE_DRAW = 0,
    UI_PAGE_BLOCK_CHECK,
    UI_PAGE_ACCURACY,
    UI_PAGE_SETTINGS,
    UI_PAGE_COUNT
} ui_page_t;

lv_obj_t *ui_create_page(lv_obj_t *parent);
void ui_create_header(lv_obj_t *page, const char *title, const char *subtitle);
lv_obj_t *ui_create_label(lv_obj_t *parent, const char *text, const lv_font_t *font, lv_color_t color);
lv_obj_t *ui_create_button(lv_obj_t *parent, const char *text, int32_t width, int32_t height);
void ui_apply_card_style(lv_obj_t *obj);
void ui_set_button_selected(lv_obj_t *button, bool selected);
void ui_info_bind_label(lv_obj_t *label);
void ui_info_focus(ui_page_t page);
void ui_info_set(ui_page_t page, const char *text);

#endif
