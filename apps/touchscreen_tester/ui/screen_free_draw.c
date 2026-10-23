/*
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "screen_free_draw.h"

#include <stdint.h>
#include <string.h>

#include "ui_common.h"

#define DRAW_CANVAS_WIDTH 404U
#define DRAW_CANVAS_HEIGHT 164U

static uint8_t s_canvas_pixels[DRAW_CANVAS_WIDTH * DRAW_CANVAS_HEIGHT];
static lv_obj_t *s_canvas;
static lv_draw_buf_t *s_canvas_draw_buf;
static lv_obj_t *s_hint_label;
static ui_header_t s_header;
static uint8_t s_line_width = 4U;
static bool s_show_coordinates;
static bool s_has_last_point;
static bool s_has_info_point;
static int32_t s_last_x;
static int32_t s_last_y;
static lv_point_t s_info_point;

static void draw_pixel(int32_t x, int32_t y)
{
    if ((x < 0) || (y < 0) || (x >= (int32_t)DRAW_CANVAS_WIDTH) ||
        (y >= (int32_t)DRAW_CANVAS_HEIGHT))
    {
        return;
    }

    s_canvas_pixels[(uint32_t)y * s_canvas_draw_buf->header.stride + (uint32_t)x] = 0U;
}

static void draw_disc(int32_t center_x, int32_t center_y, int32_t radius)
{
    int32_t y;

    for (y = -radius; y <= radius; y++)
    {
        int32_t x;

        for (x = -radius; x <= radius; x++)
        {
            if (((x * x) + (y * y)) <= (radius * radius))
            {
                draw_pixel(center_x + x, center_y + y);
            }
        }
    }
}

static void draw_segment(int32_t x0, int32_t y0, int32_t x1, int32_t y1)
{
    int32_t dx = (x1 > x0) ? (x1 - x0) : (x0 - x1);
    int32_t sx = (x0 < x1) ? 1 : -1;
    int32_t dy = (y1 > y0) ? -(y1 - y0) : -(y0 - y1);
    int32_t sy = (y0 < y1) ? 1 : -1;
    int32_t error = dx + dy;
    int32_t radius = ((int32_t)s_line_width - 1) / 2;

    for (;;)
    {
        draw_disc(x0, y0, radius);
        if ((x0 == x1) && (y0 == y1))
        {
            break;
        }

        {
            int32_t error2 = error * 2;

            if (error2 >= dy)
            {
                error += dy;
                x0 += sx;
            }
            if (error2 <= dx)
            {
                error += dx;
                y0 += sy;
            }
        }
    }
}

static void invalidate_canvas_patch(int32_t min_x, int32_t min_y, int32_t max_x, int32_t max_y)
{
    lv_area_t object_area;
    lv_area_t changed_area;
    int32_t pad = ((int32_t)s_line_width / 2) + 2;

    lv_obj_get_coords(s_canvas, &object_area);
    changed_area.x1 = object_area.x1 + min_x - pad;
    changed_area.y1 = object_area.y1 + min_y - pad;
    changed_area.x2 = object_area.x1 + max_x + pad;
    changed_area.y2 = object_area.y1 + max_y + pad;
    lv_obj_invalidate_area(s_canvas, &changed_area);
}

static void update_coordinate_label(const lv_point_t *point)
{
    char text[96];

    if (!s_show_coordinates || (point == NULL))
    {
        ui_info_set(UI_PAGE_FREE_DRAW, ui_tr(UI_TEXT_FREE_DRAW_DRAWING));
        lv_snprintf(text, sizeof(text),
                    ui_tr(UI_TEXT_FREE_DRAW_COORDS_HIDDEN_FORMAT),
                    (unsigned)s_line_width);
        ui_info_set_detail(UI_PAGE_FREE_DRAW, text);
        return;
    }

    s_info_point = *point;
    s_has_info_point = true;
    ui_info_set(UI_PAGE_FREE_DRAW, ui_tr(UI_TEXT_FREE_DRAW_TOUCH_INPUT));
    lv_snprintf(text, sizeof(text), ui_tr(UI_TEXT_FREE_DRAW_COORDS_FORMAT),
                (int)point->x, (int)point->y, (unsigned)s_line_width);
    ui_info_set_detail(UI_PAGE_FREE_DRAW, text);
}

static void update_canvas_hint(void)
{
    char text[96];

    lv_snprintf(text, sizeof(text), ui_tr(UI_TEXT_FREE_DRAW_CANVAS_FORMAT),
                (unsigned)DRAW_CANVAS_WIDTH, (unsigned)DRAW_CANVAS_HEIGHT,
                (unsigned)s_line_width);
    lv_label_set_text(s_hint_label, text);
}

static void language_changed_cb(ui_language_t language)
{
    char text[96];

    LV_UNUSED(language);
    lv_label_set_text(s_header.title, ui_tr(UI_TEXT_PAGE_FREE_DRAW_TITLE));
    lv_label_set_text(s_header.subtitle, ui_tr(UI_TEXT_PAGE_FREE_DRAW_SUBTITLE));
    update_canvas_hint();

    if (s_has_info_point && s_show_coordinates)
    {
        ui_info_set(UI_PAGE_FREE_DRAW, ui_tr(UI_TEXT_FREE_DRAW_TOUCH_INPUT));
        lv_snprintf(text, sizeof(text), ui_tr(UI_TEXT_FREE_DRAW_COORDS_FORMAT),
                    (int)s_info_point.x, (int)s_info_point.y,
                    (unsigned)s_line_width);
        ui_info_set_detail(UI_PAGE_FREE_DRAW, text);
    }
    else
    {
        ui_info_set(UI_PAGE_FREE_DRAW,
                    s_show_coordinates ?
                    ui_tr(UI_TEXT_FREE_DRAW_COORDS_SHOWN) :
                    ui_tr(UI_TEXT_FREE_DRAW_DRAWING));
        lv_snprintf(text, sizeof(text),
                    ui_tr(UI_TEXT_FREE_DRAW_WIDTH_SUMMARY_FORMAT),
                    (unsigned)s_line_width);
        ui_info_set_detail(UI_PAGE_FREE_DRAW, text);
    }
}

static void canvas_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);
    lv_obj_t *canvas = lv_event_get_target_obj(event);
    lv_area_t area;
    lv_point_t point;
    int32_t x;
    int32_t y;

    lv_obj_get_coords(canvas, &area);
    lv_indev_get_point(lv_indev_active(), &point);
    x = point.x - area.x1;
    y = point.y - area.y1;

    if ((x < 0) || (y < 0) || (x >= (int32_t)DRAW_CANVAS_WIDTH) ||
        (y >= (int32_t)DRAW_CANVAS_HEIGHT))
    {
        s_has_last_point = false;
        return;
    }

    if (code == LV_EVENT_PRESSED)
    {
        s_last_x = x;
        s_last_y = y;
        s_has_last_point = true;
        draw_disc(x, y, ((int32_t)s_line_width - 1) / 2);
        invalidate_canvas_patch(x, y, x, y);
        update_coordinate_label(&point);
    }
    else if ((code == LV_EVENT_PRESSING) && s_has_last_point)
    {
        int32_t min_x = (x < s_last_x) ? x : s_last_x;
        int32_t min_y = (y < s_last_y) ? y : s_last_y;
        int32_t max_x = (x > s_last_x) ? x : s_last_x;
        int32_t max_y = (y > s_last_y) ? y : s_last_y;

        draw_segment(s_last_x, s_last_y, x, y);
        invalidate_canvas_patch(min_x, min_y, max_x, max_y);
        s_last_x = x;
        s_last_y = y;
        update_coordinate_label(&point);
    }
    else if ((code == LV_EVENT_RELEASED) || (code == LV_EVENT_PRESS_LOST))
    {
        s_has_last_point = false;
    }
}

lv_obj_t *screen_free_draw_create(lv_obj_t *parent)
{
    lv_obj_t *page = ui_create_page(parent);
    lv_obj_t *canvas_frame;

    s_header = ui_create_header(page, ui_tr(UI_TEXT_PAGE_FREE_DRAW_TITLE),
                                ui_tr(UI_TEXT_PAGE_FREE_DRAW_SUBTITLE));

    canvas_frame = lv_obj_create(page);
    lv_obj_set_size(canvas_frame, DRAW_CANVAS_WIDTH + 2, DRAW_CANVAS_HEIGHT + 2);
    lv_obj_set_pos(canvas_frame,
                   UI_CONTENT_LEFT + (UI_CONTENT_WIDTH - (DRAW_CANVAS_WIDTH + 2)) / 2,
                   UI_CONTENT_TOP);
    ui_style_apply_canvas_frame(canvas_frame);
    lv_obj_remove_flag(canvas_frame, LV_OBJ_FLAG_SCROLLABLE);

    s_canvas = lv_canvas_create(canvas_frame);
    lv_obj_set_size(s_canvas, DRAW_CANVAS_WIDTH, DRAW_CANVAS_HEIGHT);
    lv_obj_set_pos(s_canvas, 0, 0);
    lv_canvas_set_buffer(s_canvas, s_canvas_pixels, DRAW_CANVAS_WIDTH, DRAW_CANVAS_HEIGHT,
                         LV_COLOR_FORMAT_L8);
    lv_canvas_fill_bg(s_canvas, UI_COLOR_SURFACE, LV_OPA_COVER);
    lv_obj_add_flag(s_canvas, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(s_canvas, LV_OBJ_FLAG_SCROLLABLE);
    ui_style_apply_canvas(s_canvas);
    lv_obj_add_event_cb(s_canvas, canvas_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(s_canvas, canvas_event_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(s_canvas, canvas_event_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(s_canvas, canvas_event_cb, LV_EVENT_PRESS_LOST, NULL);

    s_canvas_draw_buf = lv_canvas_get_draw_buf(s_canvas);

    s_hint_label = ui_create_label(page, "", UI_LABEL_MUTED);
    lv_obj_set_pos(s_hint_label, UI_CONTENT_LEFT, 224);
    update_canvas_hint();

    ui_info_set(UI_PAGE_FREE_DRAW, ui_tr(UI_TEXT_FREE_DRAW_READY));
    ui_info_set_detail(UI_PAGE_FREE_DRAW, ui_tr(UI_TEXT_FREE_DRAW_READY_DETAIL));
    (void)ui_i18n_subscribe(language_changed_cb);

    return page;
}

void screen_free_draw_clear(void)
{
    if (s_canvas == NULL)
    {
        return;
    }

    memset(s_canvas_pixels, 0xFF, sizeof(s_canvas_pixels));
    s_has_last_point = false;
    s_has_info_point = false;
    lv_obj_invalidate(s_canvas);
    ui_info_set(UI_PAGE_FREE_DRAW, ui_tr(UI_TEXT_FREE_DRAW_CLEARED));
    ui_info_set_detail(UI_PAGE_FREE_DRAW, ui_tr(UI_TEXT_FREE_DRAW_CLEARED_DETAIL));
}

void screen_free_draw_set_line_width(uint8_t width)
{
    char text[64];

    s_line_width = (uint8_t)((width < 2U) ? 2U : ((width > 6U) ? 6U : width));
    if ((s_canvas == NULL) || (s_hint_label == NULL))
    {
        return;
    }

    update_canvas_hint();
    ui_info_set(UI_PAGE_FREE_DRAW, ui_tr(UI_TEXT_FREE_DRAW_WIDTH_UPDATED));
    lv_snprintf(text, sizeof(text), ui_tr(UI_TEXT_FREE_DRAW_WIDTH_DETAIL_FORMAT),
                (unsigned)s_line_width);
    ui_info_set_detail(UI_PAGE_FREE_DRAW, text);
}

void screen_free_draw_set_show_coordinates(bool show)
{
    char text[96];

    s_show_coordinates = show;
    ui_info_set(UI_PAGE_FREE_DRAW,
                show ? ui_tr(UI_TEXT_FREE_DRAW_COORDS_SHOWN) :
                       ui_tr(UI_TEXT_FREE_DRAW_COORDS_HIDDEN));
    lv_snprintf(text, sizeof(text),
                ui_tr(UI_TEXT_FREE_DRAW_WIDTH_SUMMARY_FORMAT),
                (unsigned)s_line_width);
    ui_info_set_detail(UI_PAGE_FREE_DRAW, text);
}
