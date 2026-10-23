/*
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ui_common.h"

lv_obj_t *ui_create_page(lv_obj_t *parent)
{
    lv_obj_t *page = lv_obj_create(parent);

    lv_obj_set_size(page, UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT);
    lv_obj_set_pos(page, 0, 0);
    lv_obj_set_style_bg_color(page, UI_COLOR_BG, 0);
    lv_obj_set_style_bg_opa(page, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(page, 0, 0);
    lv_obj_set_style_pad_all(page, 0, 0);
    lv_obj_set_style_radius(page, 0, 0);
    lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);

    return page;
}

lv_obj_t *ui_create_label(lv_obj_t *parent, const char *text, const lv_font_t *font, lv_color_t color)
{
    lv_obj_t *label = lv_label_create(parent);

    lv_label_set_text(label, text);
    lv_obj_set_style_text_font(label, font, 0);
    lv_obj_set_style_text_color(label, color, 0);

    return label;
}

void ui_create_header(lv_obj_t *page, const char *title, const char *subtitle)
{
    lv_obj_t *title_label = ui_create_label(page, title, &lv_font_montserrat_24, UI_COLOR_INK);
    lv_obj_set_pos(title_label, UI_CONTENT_LEFT, 2);

    lv_obj_t *subtitle_label = ui_create_label(page, subtitle, &lv_font_montserrat_16, UI_COLOR_MUTED);
    lv_obj_set_pos(subtitle_label, UI_CONTENT_LEFT, 33);
}

lv_obj_t *ui_create_button(lv_obj_t *parent, const char *text, int32_t width, int32_t height)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_t *label = ui_create_label(button, text, &lv_font_montserrat_16, lv_color_white());

    lv_obj_set_size(button, width, height);
    lv_obj_set_style_bg_color(button, UI_COLOR_ACCENT, 0);
    lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(button, 0, 0);
    lv_obj_set_style_radius(button, 9, 0);
    lv_obj_set_style_pad_all(button, 0, 0);
    lv_obj_center(label);

    return button;
}

void ui_apply_card_style(lv_obj_t *obj)
{
    lv_obj_set_style_bg_color(obj, UI_COLOR_SURFACE, 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(obj, UI_COLOR_BORDER, 0);
    lv_obj_set_style_border_width(obj, 1, 0);
    lv_obj_set_style_radius(obj, 12, 0);
    lv_obj_set_style_pad_all(obj, 12, 0);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
}

void ui_set_button_selected(lv_obj_t *button, bool selected)
{
    lv_obj_set_style_bg_color(button, selected ? UI_COLOR_ACCENT : UI_COLOR_SURFACE, 0);
    lv_obj_set_style_border_color(button, selected ? UI_COLOR_ACCENT : UI_COLOR_BORDER, 0);
    lv_obj_set_style_border_width(button, selected ? 0 : 1, 0);

    lv_obj_t *label = lv_obj_get_child(button, 0);
    if (label != NULL)
    {
        lv_obj_set_style_text_color(label, selected ? lv_color_white() : UI_COLOR_INK, 0);
    }
}
