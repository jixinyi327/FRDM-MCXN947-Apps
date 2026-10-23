/*
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "ui_common.h"

static lv_obj_t *s_info_primary_label;
static lv_obj_t *s_info_detail_label;
static char s_info_primary[UI_PAGE_COUNT][64];
static char s_info_detail[UI_PAGE_COUNT][96];
static ui_page_t s_info_page;

lv_obj_t *ui_create_page(lv_obj_t *parent)
{
    lv_obj_t *page = lv_obj_create(parent);

    lv_obj_set_size(page, UI_SCREEN_WIDTH, UI_SCREEN_HEIGHT);
    lv_obj_set_pos(page, 0, 0);
    ui_style_apply_screen(page);
    lv_obj_remove_flag(page, LV_OBJ_FLAG_SCROLLABLE);

    return page;
}

lv_obj_t *ui_create_label(lv_obj_t *parent, const char *text, ui_label_role_t role)
{
    lv_obj_t *label = lv_label_create(parent);

    lv_label_set_text(label, text);
    ui_style_apply_label(label, role);

    return label;
}

void ui_create_header(lv_obj_t *page, const char *title, const char *subtitle)
{
    lv_obj_t *title_label = ui_create_label(page, title, UI_LABEL_TITLE);
    lv_obj_set_pos(title_label, UI_CONTENT_LEFT, 2);

    lv_obj_t *subtitle_label = ui_create_label(page, subtitle, UI_LABEL_MUTED);
    lv_obj_set_pos(subtitle_label, UI_CONTENT_LEFT, 33);
}

lv_obj_t *ui_create_button_base(lv_obj_t *parent, const char *text, int32_t width, int32_t height)
{
    lv_obj_t *button = lv_button_create(parent);
    lv_obj_t *label = ui_create_label(button, text, UI_LABEL_BUTTON);

    lv_obj_set_size(button, width, height);
    lv_obj_center(label);

    return button;
}

lv_obj_t *ui_create_button(lv_obj_t *parent, const char *text, int32_t width, int32_t height)
{
    lv_obj_t *button = ui_create_button_base(parent, text, width, height);

    ui_style_apply_button_primary(button);
    return button;
}

void ui_set_button_selected(lv_obj_t *button, bool selected)
{
    if (selected)
    {
        lv_obj_add_state(button, LV_STATE_CHECKED);
    }
    else
    {
        lv_obj_remove_state(button, LV_STATE_CHECKED);
    }
}

void ui_info_bind_labels(lv_obj_t *primary_label, lv_obj_t *detail_label)
{
    s_info_primary_label = primary_label;
    s_info_detail_label = detail_label;
}

void ui_info_focus(ui_page_t page)
{
    if (page >= UI_PAGE_COUNT)
    {
        return;
    }

    s_info_page = page;
    if (s_info_primary_label != NULL)
    {
        lv_label_set_text(s_info_primary_label, s_info_primary[page]);
    }
    if (s_info_detail_label != NULL)
    {
        lv_label_set_text(s_info_detail_label, s_info_detail[page]);
    }
}

void ui_info_set(ui_page_t page, const char *text)
{
    if ((page >= UI_PAGE_COUNT) || (text == NULL))
    {
        return;
    }

    lv_snprintf(s_info_primary[page], sizeof(s_info_primary[page]), "%s", text);
    if ((page == s_info_page) && (s_info_primary_label != NULL))
    {
        lv_label_set_text(s_info_primary_label, s_info_primary[page]);
    }
}

void ui_info_set_detail(ui_page_t page, const char *text)
{
    if ((page >= UI_PAGE_COUNT) || (text == NULL))
    {
        return;
    }

    lv_snprintf(s_info_detail[page], sizeof(s_info_detail[page]), "%s", text);
    if ((page == s_info_page) && (s_info_detail_label != NULL))
    {
        lv_label_set_text(s_info_detail_label, s_info_detail[page]);
    }
}
