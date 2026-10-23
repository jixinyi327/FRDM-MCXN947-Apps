/*
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "screen_settings.h"

#include <stdint.h>

#include "screen_free_draw.h"
#include "ui_common.h"

#define WIDTH_BUTTON_COUNT 3U

static lv_obj_t *s_width_buttons[WIDTH_BUTTON_COUNT];
static lv_obj_t *s_coordinate_switch;
static lv_obj_t *s_coordinate_knob;
static lv_obj_t *s_width_label;
static lv_obj_t *s_coordinate_label;
static lv_obj_t *s_language_label;
static lv_obj_t *s_language_button;
static ui_header_t s_header;
static bool s_show_coordinates;

static void knob_anim_cb(void *var, int32_t x)
{
    lv_obj_set_x((lv_obj_t *)var, x);
}

static void update_width_buttons(uint8_t selected_width)
{
    uint32_t i;

    for (i = 0; i < WIDTH_BUTTON_COUNT; i++)
    {
        uint8_t width = (uint8_t)(2U + (i * 2U));
        ui_set_button_selected(s_width_buttons[i], width == selected_width);
    }
}

static void width_event_cb(lv_event_t *event)
{
    uint8_t width = (uint8_t)(uintptr_t)lv_event_get_user_data(event);

    screen_free_draw_set_line_width(width);
    update_width_buttons(width);
    {
        char text[48];

        lv_snprintf(text, sizeof(text), ui_tr(UI_TEXT_SETTINGS_WIDTH_DETAIL_FORMAT),
                    (unsigned)width);
        ui_info_set(UI_PAGE_SETTINGS, ui_tr(UI_TEXT_SETTINGS_WIDTH_UPDATED));
        ui_info_set_detail(UI_PAGE_SETTINGS, text);
    }
}

static void update_coordinate_switch(bool animate)
{
    lv_anim_t animation;
    int32_t target_x = s_show_coordinates ? 26 : 2;

    if (s_show_coordinates)
    {
        lv_obj_add_state(s_coordinate_switch, LV_STATE_CHECKED);
    }
    else
    {
        lv_obj_remove_state(s_coordinate_switch, LV_STATE_CHECKED);
    }

    lv_anim_delete(s_coordinate_knob, knob_anim_cb);
    if (!animate)
    {
        lv_obj_set_x(s_coordinate_knob, target_x);
        return;
    }

    lv_anim_init(&animation);
    lv_anim_set_var(&animation, s_coordinate_knob);
    lv_anim_set_values(&animation, lv_obj_get_x(s_coordinate_knob), target_x);
    lv_anim_set_duration(&animation, 150);
    lv_anim_set_path_cb(&animation, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&animation, knob_anim_cb);
    lv_anim_start(&animation);
}

static void coordinate_switch_event_cb(lv_event_t *event)
{
    LV_UNUSED(event);

    s_show_coordinates = !s_show_coordinates;
    screen_free_draw_set_show_coordinates(s_show_coordinates);
    update_coordinate_switch(true);
    ui_info_set(UI_PAGE_SETTINGS,
                s_show_coordinates ?
                ui_tr(UI_TEXT_SETTINGS_COORDS_SHOWN) :
                ui_tr(UI_TEXT_SETTINGS_COORDS_HIDDEN));
    ui_info_set_detail(UI_PAGE_SETTINGS, ui_tr(UI_TEXT_SETTINGS_COORDS_DETAIL));
}

static lv_obj_t *create_card(lv_obj_t *page, int32_t y, int32_t height)
{
    lv_obj_t *card = lv_obj_create(page);

    lv_obj_set_size(card, UI_CONTENT_WIDTH, height);
    lv_obj_set_pos(card, UI_CONTENT_LEFT, y);
    ui_style_apply_settings_card(card);

    return card;
}

static void create_width_buttons(lv_obj_t *card)
{
    static const char *labels[WIDTH_BUTTON_COUNT] = {"2", "4", "6"};
    lv_obj_t *label = ui_create_label(card, ui_tr(UI_TEXT_SETTINGS_LINE_WIDTH_LABEL),
                                      UI_LABEL_BODY);
    uint32_t i;

    lv_obj_set_pos(label, 0, 7);
    s_width_label = label;

    for (i = 0; i < WIDTH_BUTTON_COUNT; i++)
    {
        s_width_buttons[i] = ui_create_button_base(card, labels[i], 46, 34);
        ui_style_apply_option_button(s_width_buttons[i]);
        lv_obj_set_pos(s_width_buttons[i], 250 + (int32_t)i * 54, 0);
        lv_obj_set_ext_click_area(s_width_buttons[i], 4);
        lv_obj_add_event_cb(s_width_buttons[i], width_event_cb, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)(2U + (i * 2U)));
    }

    update_width_buttons(4U);
}

static void create_coordinate_switch(lv_obj_t *card)
{
    lv_obj_t *label = ui_create_label(card, ui_tr(UI_TEXT_SETTINGS_COORDINATES_LABEL),
                                      UI_LABEL_BODY);

    lv_obj_set_pos(label, 0, 5);
    s_coordinate_label = label;

    s_coordinate_switch = lv_obj_create(card);
    lv_obj_set_size(s_coordinate_switch, 56, 30);
    lv_obj_set_pos(s_coordinate_switch, 346, 0);
    ui_style_apply_switch_track(s_coordinate_switch);
    lv_obj_set_ext_click_area(s_coordinate_switch, 6);
    lv_obj_add_flag(s_coordinate_switch, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(s_coordinate_switch, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_coordinate_switch, coordinate_switch_event_cb, LV_EVENT_CLICKED, NULL);

    s_coordinate_knob = lv_obj_create(s_coordinate_switch);
    lv_obj_set_size(s_coordinate_knob, 26, 26);
    lv_obj_set_pos(s_coordinate_knob, 2, 2);
    ui_style_apply_switch_knob(s_coordinate_knob);
    lv_obj_remove_flag(s_coordinate_knob, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(s_coordinate_knob, LV_OBJ_FLAG_CLICKABLE);

    s_show_coordinates = false;
    update_coordinate_switch(false);
}

static void language_button_event_cb(lv_event_t *event)
{
    LV_UNUSED(event);
    ui_i18n_toggle();
}

static void create_language_card(lv_obj_t *card)
{
    s_language_label = ui_create_label(card, ui_tr(UI_TEXT_SETTINGS_LANGUAGE_LABEL),
                                       UI_LABEL_BODY);
    lv_obj_set_pos(s_language_label, 0, 5);

    s_language_button = ui_create_button(card, ui_tr(UI_TEXT_SETTINGS_LANGUAGE_TOGGLE),
                                         116, 40);
    lv_obj_set_pos(s_language_button, 300, 0);
    lv_obj_add_event_cb(s_language_button, language_button_event_cb,
                        LV_EVENT_CLICKED, NULL);
}

static void language_changed_cb(ui_language_t language)
{
    LV_UNUSED(language);
    lv_label_set_text(s_header.title, ui_tr(UI_TEXT_PAGE_SETTINGS_TITLE));
    lv_label_set_text(s_header.subtitle, ui_tr(UI_TEXT_PAGE_SETTINGS_SUBTITLE));
    lv_label_set_text(s_width_label, ui_tr(UI_TEXT_SETTINGS_LINE_WIDTH_LABEL));
    lv_label_set_text(s_coordinate_label, ui_tr(UI_TEXT_SETTINGS_COORDINATES_LABEL));
    lv_label_set_text(s_language_label, ui_tr(UI_TEXT_SETTINGS_LANGUAGE_LABEL));
    lv_label_set_text(lv_obj_get_child(s_language_button, 0),
                      ui_tr(UI_TEXT_SETTINGS_LANGUAGE_TOGGLE));
    ui_info_set(UI_PAGE_SETTINGS, ui_tr(UI_TEXT_SETTINGS_READY));
    ui_info_set_detail(UI_PAGE_SETTINGS, ui_tr(UI_TEXT_SETTINGS_READY_DETAIL));
}

lv_obj_t *screen_settings_create(lv_obj_t *parent)
{
    lv_obj_t *page = ui_create_page(parent);
    lv_obj_t *drawing_card;
    lv_obj_t *input_card;
    lv_obj_t *language_card;

    s_header = ui_create_header(page, ui_tr(UI_TEXT_PAGE_SETTINGS_TITLE),
                                ui_tr(UI_TEXT_PAGE_SETTINGS_SUBTITLE));

    drawing_card = create_card(page, 54, 56);
    input_card = create_card(page, 116, 56);
    language_card = create_card(page, 178, 64);

    create_width_buttons(drawing_card);
    create_coordinate_switch(input_card);
    create_language_card(language_card);

    ui_info_set(UI_PAGE_SETTINGS, ui_tr(UI_TEXT_SETTINGS_READY));
    ui_info_set_detail(UI_PAGE_SETTINGS, ui_tr(UI_TEXT_SETTINGS_READY_DETAIL));
    (void)ui_i18n_subscribe(language_changed_cb);

    return page;
}
