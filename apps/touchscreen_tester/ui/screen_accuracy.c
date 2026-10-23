/*
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "screen_accuracy.h"

#include <stdint.h>

#include "ui_common.h"

#define TARGET_SIZE 56
#define TARGET_HIT_RADIUS (TARGET_SIZE / 2)
#define TARGET_INSET 10
#define TARGET_ANIM_MS 260
#define ERROR_SHAKE_MS 360
#define ERROR_SHAKE_AMPLITUDE 10

typedef struct {
    lv_obj_t *target;
    int32_t start_x;
    int32_t start_y;
    int32_t end_x;
    int32_t end_y;
} target_anim_context_t;

static lv_obj_t *s_area;
static lv_obj_t *s_target;
static lv_obj_t *s_target_marks[3];
static target_anim_context_t s_target_anim_context;
static lv_anim_t s_target_anim;
static lv_anim_t s_error_anim;
static int32_t s_target_x;
static int32_t s_target_y;
static uint32_t s_attempt_count;
static uint32_t s_total_error_tenths;
static uint32_t s_best_error_tenths;
static uint32_t s_random_state = 0xC0FFEEU;
static bool s_target_moving;
static bool s_error_feedback_active;

static uint32_t next_random(void)
{
    s_random_state = s_random_state * 1664525U + 1013904223U;
    return s_random_state;
}

static lv_obj_t *create_target_mark(lv_obj_t *parent, int32_t width, int32_t height)
{
    lv_obj_t *mark = lv_obj_create(parent);

    lv_obj_set_size(mark, width, height);
    lv_obj_set_style_bg_color(mark, UI_COLOR_ACCENT, 0);
    lv_obj_set_style_bg_opa(mark, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(mark, 0, 0);
    lv_obj_set_style_radius(mark, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_all(mark, 0, 0);
    lv_obj_remove_flag(mark, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(mark, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_align(mark, LV_ALIGN_CENTER, 0, 0);

    return mark;
}

static void target_anim_cb(void *var, int32_t progress)
{
    target_anim_context_t *context = (target_anim_context_t *)var;
    int32_t x = context->start_x +
                ((context->end_x - context->start_x) * progress) / 1000;
    int32_t y = context->start_y +
                ((context->end_y - context->start_y) * progress) / 1000;

    s_target_x = x;
    s_target_y = y;
    lv_obj_set_pos(context->target, x, y);
}

static void target_anim_completed(lv_anim_t *animation)
{
    LV_UNUSED(animation);
    s_target_x = s_target_anim_context.end_x;
    s_target_y = s_target_anim_context.end_y;
    s_target_moving = false;
}

static void move_target_to_random_position(void)
{
    int32_t max_x = UI_CONTENT_WIDTH - TARGET_SIZE - TARGET_INSET;
    int32_t max_y = UI_CONTENT_HEIGHT - TARGET_SIZE - TARGET_INSET;
    int32_t range_x = max_x - TARGET_INSET + 1;
    int32_t range_y = max_y - TARGET_INSET + 1;

    lv_anim_delete(&s_target_anim_context, target_anim_cb);
    s_target_moving = true;
    s_target_anim_context.target = s_target;
    s_target_anim_context.start_x = s_target_x;
    s_target_anim_context.start_y = s_target_y;
    s_target_anim_context.end_x = TARGET_INSET + (int32_t)(next_random() % (uint32_t)range_x);
    s_target_anim_context.end_y = TARGET_INSET + (int32_t)(next_random() % (uint32_t)range_y);

    lv_anim_init(&s_target_anim);
    lv_anim_set_var(&s_target_anim, &s_target_anim_context);
    lv_anim_set_values(&s_target_anim, 0, 1000);
    lv_anim_set_duration(&s_target_anim, TARGET_ANIM_MS);
    lv_anim_set_path_cb(&s_target_anim, lv_anim_path_ease_in_out);
    lv_anim_set_exec_cb(&s_target_anim, target_anim_cb);
    lv_anim_set_completed_cb(&s_target_anim, target_anim_completed);
    lv_anim_start(&s_target_anim);
}

static void set_target_color(lv_color_t color)
{
    uint32_t i;

    lv_obj_set_style_border_color(s_target, color, 0);
    for (i = 0; i < 3U; i++)
    {
        lv_obj_set_style_bg_color(s_target_marks[i], color, 0);
    }
}

static void error_shake_anim_cb(void *var, int32_t progress)
{
    lv_obj_t *target = (lv_obj_t *)var;
    int32_t angle = (progress * 1080) / 1000;
    int32_t amplitude = (ERROR_SHAKE_AMPLITUDE * (1000 - progress)) / 1000;
    int32_t offset = (lv_trigo_sin((int16_t)angle) * amplitude) / 32767;
    uint32_t red_mix;

    lv_obj_set_style_translate_x(target, offset, 0);
    if (progress < 350)
    {
        red_mix = (uint32_t)progress * 255U / 350U;
    }
    else if (progress < 600)
    {
        red_mix = 255U;
    }
    else
    {
        red_mix = (uint32_t)(1000 - progress) * 255U / 400U;
    }
    set_target_color(lv_color_mix(UI_COLOR_DANGER, UI_COLOR_ACCENT, (uint8_t)red_mix));
}

static void error_shake_completed(lv_anim_t *animation)
{
    LV_UNUSED(animation);
    lv_obj_set_style_translate_x(s_target, 0, 0);
    set_target_color(UI_COLOR_ACCENT);
    s_error_feedback_active = false;
}

static void show_error_feedback(void)
{
    lv_anim_delete(s_target, error_shake_anim_cb);
    s_error_feedback_active = true;

    lv_anim_init(&s_error_anim);
    lv_anim_set_var(&s_error_anim, s_target);
    lv_anim_set_values(&s_error_anim, 0, 1000);
    lv_anim_set_duration(&s_error_anim, ERROR_SHAKE_MS);
    lv_anim_set_exec_cb(&s_error_anim, error_shake_anim_cb);
    lv_anim_set_completed_cb(&s_error_anim, error_shake_completed);
    lv_anim_start(&s_error_anim);
}

static void update_stats(uint32_t error_tenths, bool hit)
{
    char text[96];
    uint32_t average_tenths;

    s_attempt_count++;
    s_total_error_tenths += error_tenths;
    if (error_tenths < s_best_error_tenths)
    {
        s_best_error_tenths = error_tenths;
    }

    average_tenths = s_total_error_tenths / s_attempt_count;
    if (hit)
    {
        lv_snprintf(text, sizeof(text),
                    "HIT %u.%u px",
                    (unsigned)(error_tenths / 10U), (unsigned)(error_tenths % 10U));
        ui_info_set(UI_PAGE_ACCURACY, text);
        lv_snprintf(text, sizeof(text),
                    "Avg %u.%u | Best %u.%u px | N %u",
                    (unsigned)(average_tenths / 10U), (unsigned)(average_tenths % 10U),
                    (unsigned)(s_best_error_tenths / 10U), (unsigned)(s_best_error_tenths % 10U),
                    (unsigned)s_attempt_count);
        ui_info_set_detail(UI_PAGE_ACCURACY, text);
    }
    else
    {
        lv_snprintf(text, sizeof(text), "MISS %u.%u px",
                    (unsigned)(error_tenths / 10U), (unsigned)(error_tenths % 10U));
        ui_info_set(UI_PAGE_ACCURACY, text);
        ui_info_set_detail(UI_PAGE_ACCURACY, "Tap inside target and try again");
    }
}

static void area_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);

    if (code == LV_EVENT_PRESSED)
    {
        lv_area_t area_coords;
        lv_point_t point;
        int32_t local_x;
        int32_t local_y;
        int32_t center_x;
        int32_t center_y;
        int32_t dx;
        int32_t dy;
        uint32_t distance;
        uint32_t distance_tenths;
        bool hit;

        if (s_target_moving || s_error_feedback_active)
        {
            return;
        }

        lv_obj_get_coords(s_area, &area_coords);
        lv_indev_get_point(lv_indev_active(), &point);
        local_x = point.x - area_coords.x1;
        local_y = point.y - area_coords.y1;
        center_x = s_target_x + (TARGET_SIZE / 2);
        center_y = s_target_y + (TARGET_SIZE / 2);
        dx = local_x - center_x;
        dy = local_y - center_y;
        distance = (uint32_t)lv_sqrt32((uint32_t)((dx * dx) + (dy * dy)));
        distance_tenths = distance * 10U;
        hit = distance <= TARGET_HIT_RADIUS;

        update_stats(distance_tenths, hit);
        if (hit)
        {
            move_target_to_random_position();
        }
        else
        {
            show_error_feedback();
        }
    }
}

lv_obj_t *screen_accuracy_create(lv_obj_t *parent)
{
    lv_obj_t *page = ui_create_page(parent);
    lv_obj_t *horizontal_mark;
    lv_obj_t *vertical_mark;
    lv_obj_t *center_mark;

    ui_create_header(page, "Touch Accuracy", "Tap the center of each moving target");

    s_area = lv_obj_create(page);
    lv_obj_set_size(s_area, UI_CONTENT_WIDTH, UI_CONTENT_HEIGHT);
    lv_obj_set_pos(s_area, UI_CONTENT_LEFT, UI_CONTENT_TOP);
    ui_apply_card_style(s_area);
    lv_obj_set_style_pad_all(s_area, 0, 0);
    lv_obj_add_flag(s_area, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_area, area_event_cb, LV_EVENT_PRESSED, NULL);

    s_target = lv_obj_create(s_area);
    lv_obj_set_size(s_target, TARGET_SIZE, TARGET_SIZE);
    lv_obj_set_style_bg_opa(s_target, LV_OPA_TRANSP, 0);
    lv_obj_set_style_border_color(s_target, UI_COLOR_ACCENT, 0);
    lv_obj_set_style_border_width(s_target, 3, 0);
    lv_obj_set_style_radius(s_target, LV_RADIUS_CIRCLE, 0);
    lv_obj_set_style_pad_all(s_target, 0, 0);
    lv_obj_remove_flag(s_target, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_remove_flag(s_target, LV_OBJ_FLAG_CLICKABLE);

    horizontal_mark = create_target_mark(s_target, 22, 2);
    vertical_mark = create_target_mark(s_target, 2, 22);
    center_mark = create_target_mark(s_target, 6, 6);
    s_target_marks[0] = horizontal_mark;
    s_target_marks[1] = vertical_mark;
    s_target_marks[2] = center_mark;

    s_target_x = (UI_CONTENT_WIDTH - TARGET_SIZE) / 2;
    s_target_y = (UI_CONTENT_HEIGHT - TARGET_SIZE) / 2;
    lv_obj_set_pos(s_target, s_target_x, s_target_y);

    screen_accuracy_reset();
    return page;
}

void screen_accuracy_reset(void)
{
    if ((s_area == NULL) || (s_target == NULL))
    {
        return;
    }

    lv_anim_delete(&s_target_anim_context, target_anim_cb);
    lv_anim_delete(s_target, error_shake_anim_cb);
    s_target_moving = false;
    s_error_feedback_active = false;
    lv_obj_set_style_translate_x(s_target, 0, 0);
    set_target_color(UI_COLOR_ACCENT);
    s_attempt_count = 0U;
    s_total_error_tenths = 0U;
    s_best_error_tenths = UINT32_MAX;
    s_target_x = (UI_CONTENT_WIDTH - TARGET_SIZE) / 2;
    s_target_y = (UI_CONTENT_HEIGHT - TARGET_SIZE) / 2;
    lv_obj_set_pos(s_target, s_target_x, s_target_y);
    ui_info_set(UI_PAGE_ACCURACY, "Accuracy reset");
    ui_info_set_detail(UI_PAGE_ACCURACY, "Tap the moving target to begin");
}
