/*
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "screen_settings.h"

#include <stdint.h>

#include "screen_accuracy.h"
#include "screen_block_check.h"
#include "screen_free_draw.h"
#include "ui_common.h"

#define WIDTH_BUTTON_COUNT 3U

static lv_obj_t *s_width_buttons[WIDTH_BUTTON_COUNT];
static lv_obj_t *s_coordinate_switch;
static lv_obj_t *s_coordinate_knob;
static lv_obj_t *s_action_status;
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
}

static void update_coordinate_switch(bool animate)
{
    lv_anim_t animation;
    int32_t target_x = s_show_coordinates ? 26 : 2;

    lv_obj_set_style_bg_color(s_coordinate_switch,
                              s_show_coordinates ? UI_COLOR_ACCENT : UI_COLOR_BORDER, 0);

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
}

static void reset_tests_event_cb(lv_event_t *event)
{
    LV_UNUSED(event);

    screen_free_draw_clear();
    screen_block_check_reset();
    screen_accuracy_reset();
    lv_label_set_text(s_action_status, "All test data reset");
}

static lv_obj_t *create_card(lv_obj_t *page, int32_t y, int32_t height)
{
    lv_obj_t *card = lv_obj_create(page);

    lv_obj_set_size(card, UI_CONTENT_WIDTH, height);
    lv_obj_set_pos(card, UI_CONTENT_LEFT, y);
    ui_apply_card_style(card);
    lv_obj_set_style_pad_ver(card, 10, 0);

    return card;
}

static void create_width_buttons(lv_obj_t *card)
{
    static const char *labels[WIDTH_BUTTON_COUNT] = {"2", "4", "6"};
    lv_obj_t *label = ui_create_label(card, "Draw line width", &lv_font_montserrat_16, UI_COLOR_INK);
    uint32_t i;

    lv_obj_set_pos(label, 0, 7);

    for (i = 0; i < WIDTH_BUTTON_COUNT; i++)
    {
        s_width_buttons[i] = ui_create_button(card, labels[i], 46, 34);
        lv_obj_set_pos(s_width_buttons[i], 250 + (int32_t)i * 54, 0);
        lv_obj_add_event_cb(s_width_buttons[i], width_event_cb, LV_EVENT_CLICKED,
                            (void *)(uintptr_t)(2U + (i * 2U)));
    }

    update_width_buttons(4U);
}

static void create_coordinate_switch(lv_obj_t *card)
{
    lv_obj_t *label = ui_create_label(card, "Show touch coordinates",
                                      &lv_font_montserrat_16, UI_COLOR_INK);

    lv_obj_set_pos(label, 0, 5);

    s_coordinate_switch = lv_obj_create(card);
    lv_obj_set_size(s_coordinate_switch, 56, 30);
    lv_obj_set_pos(s_coordinate_switch, 346, 0);
    lv_obj_set_style_radius(s_coordinate_switch, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_border_width(s_coordinate_switch, 0, 0);
    lv_obj_set_style_bg_opa(s_coordinate_switch, LV_OPA_COVER, 0);
    lv_obj_set_style_pad_all(s_coordinate_switch, 0, 0);
    lv_obj_add_flag(s_coordinate_switch, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(s_coordinate_switch, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_add_event_cb(s_coordinate_switch, coordinate_switch_event_cb, LV_EVENT_CLICKED, NULL);

    s_coordinate_knob = lv_obj_create(s_coordinate_switch);
    lv_obj_set_size(s_coordinate_knob, 26, 26);
    lv_obj_set_pos(s_coordinate_knob, 2, 2);
    lv_obj_set_style_bg_color(s_coordinate_knob, lv_color_white(), 0);
    lv_obj_set_style_bg_opa(s_coordinate_knob, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(s_coordinate_knob, 0, 0);
    lv_obj_set_style_radius(s_coordinate_knob, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_all(s_coordinate_knob, 0, 0);
    lv_obj_remove_flag(s_coordinate_knob, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(s_coordinate_knob, LV_OBJ_FLAG_CLICKABLE);

    s_show_coordinates = false;
    update_coordinate_switch(false);
}

static void create_maintenance_card(lv_obj_t *card)
{
    lv_obj_t *label = ui_create_label(card, "Clear drawing and test history",
                                      &lv_font_montserrat_16, UI_COLOR_INK);
    lv_obj_t *reset_button;

    lv_obj_set_pos(label, 0, 5);

    reset_button = ui_create_button(card, "Reset tests", 112, 34);
    lv_obj_set_pos(reset_button, 280, 0);
    lv_obj_set_style_bg_color(reset_button, UI_COLOR_DANGER, 0);
    lv_obj_add_event_cb(reset_button, reset_tests_event_cb, LV_EVENT_CLICKED, NULL);
}

lv_obj_t *screen_settings_create(lv_obj_t *parent)
{
    lv_obj_t *page = ui_create_page(parent);
    lv_obj_t *drawing_card;
    lv_obj_t *input_card;
    lv_obj_t *maintenance_card;
    lv_obj_t *build_label;

    ui_create_header(page, "Settings", "Adjust test presentation and reset test state");

    drawing_card = create_card(page, 54, 56);
    input_card = create_card(page, 116, 56);
    maintenance_card = create_card(page, 178, 56);

    create_width_buttons(drawing_card);
    create_coordinate_switch(input_card);
    create_maintenance_card(maintenance_card);

    s_action_status = ui_create_label(page, "Changes apply immediately",
                                      &lv_font_montserrat_16, UI_COLOR_MUTED);
    lv_obj_set_pos(s_action_status, UI_CONTENT_LEFT, 240);

    build_label = ui_create_label(page,
                                  "UI core cm33_core0  |  ST7796S  |  GT911",
                                  &lv_font_montserrat_16, UI_COLOR_MUTED);
    lv_obj_set_pos(build_label, UI_CONTENT_LEFT, 278);
    LV_UNUSED(build_label);

    return page;
}
