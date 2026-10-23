/*
 * Shared interaction and layout primitives for the touchscreen tester.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UI_COMMON_H
#define UI_COMMON_H

#include "lvgl.h"
#include "ui_theme.h"

#define UI_SCREEN_WIDTH 480
#define UI_SCREEN_HEIGHT 320
#define UI_HEADER_HEIGHT 52
#define UI_CONTENT_LEFT 12
#define UI_CONTENT_TOP 54
#define UI_CONTENT_WIDTH 456
#define UI_CONTENT_HEIGHT 198

#define UI_FOOTER_MARGIN 12
#define UI_FOOTER_HEIGHT 48
#define UI_FOOTER_BUTTON_WIDTH 72
#define UI_FOOTER_GAP 8
#define UI_FOOTER_TOP (UI_SCREEN_HEIGHT - UI_FOOTER_MARGIN - UI_FOOTER_HEIGHT)

typedef enum {
    UI_PAGE_FREE_DRAW = 0,
    UI_PAGE_BLOCK_CHECK,
    UI_PAGE_ACCURACY,
    UI_PAGE_SETTINGS,
    UI_PAGE_COUNT
} ui_page_t;

typedef struct {
    lv_obj_t *title;
    lv_obj_t *subtitle;
} ui_header_t;

lv_obj_t *ui_create_page(lv_obj_t *parent);
ui_header_t ui_create_header(lv_obj_t *page, const char *title, const char *subtitle);
lv_obj_t *ui_create_label(lv_obj_t *parent, const char *text, ui_label_role_t role);
lv_obj_t *ui_create_button_base(lv_obj_t *parent, const char *text, int32_t width, int32_t height);
lv_obj_t *ui_create_button(lv_obj_t *parent, const char *text, int32_t width, int32_t height);
void ui_set_button_selected(lv_obj_t *button, bool selected);
void ui_info_bind_labels(lv_obj_t *primary_label, lv_obj_t *detail_label);
void ui_info_focus(ui_page_t page);
void ui_info_set(ui_page_t page, const char *text);
void ui_info_set_detail(ui_page_t page, const char *text);

#endif
