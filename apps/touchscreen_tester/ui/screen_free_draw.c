/*
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "screen_free_draw.h"

#include <stdint.h>
#include <string.h>

#include "ui_common.h"

#define DRAW_CANVAS_WIDTH 400U
#define DRAW_CANVAS_HEIGHT 160U

static uint8_t s_canvas_pixels[DRAW_CANVAS_WIDTH * DRAW_CANVAS_HEIGHT];
static lv_obj_t *s_canvas;
static lv_draw_buf_t *s_canvas_draw_buf;
static lv_obj_t *s_hint_label;
static lv_obj_t *s_coordinate_label;
static uint8_t s_line_width = 4U;
static bool s_show_coordinates;
static bool s_has_last_point;
static int32_t s_last_x;
static int32_t s_last_y;

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
    char text[48];

    if (!s_show_coordinates || (point == NULL))
    {
        lv_label_set_text(s_coordinate_label, "Coordinates hidden");
        return;
    }

    lv_snprintf(text, sizeof(text), "x:%3d  y:%3d", (int)point->x, (int)point->y);
    lv_label_set_text(s_coordinate_label, text);
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

static void clear_event_cb(lv_event_t *event)
{
    LV_UNUSED(event);
    screen_free_draw_clear();
}

lv_obj_t *screen_free_draw_create(lv_obj_t *parent)
{
    lv_obj_t *page = ui_create_page(parent);
    lv_obj_t *clear_button;
    lv_obj_t *hint_label;
    lv_obj_t *canvas_frame;

    ui_create_header(page, "Free Draw", "Draw continuously to inspect touch continuity");

    clear_button = ui_create_button(page, "Clear", 82, 34);
    lv_obj_align(clear_button, LV_ALIGN_TOP_RIGHT, -20, 8);
    lv_obj_add_event_cb(clear_button, clear_event_cb, LV_EVENT_CLICKED, NULL);

    canvas_frame = lv_obj_create(page);
    lv_obj_set_size(canvas_frame, DRAW_CANVAS_WIDTH + 2, DRAW_CANVAS_HEIGHT + 2);
    lv_obj_set_pos(canvas_frame, UI_CONTENT_LEFT, UI_CONTENT_TOP);
    ui_apply_card_style(canvas_frame);
    lv_obj_set_style_pad_all(canvas_frame, 0, 0);
    lv_obj_set_style_radius(canvas_frame, 8, 0);

    s_canvas = lv_canvas_create(canvas_frame);
    lv_obj_set_size(s_canvas, DRAW_CANVAS_WIDTH, DRAW_CANVAS_HEIGHT);
    lv_obj_set_pos(s_canvas, 0, 0);
    lv_canvas_set_buffer(s_canvas, s_canvas_pixels, DRAW_CANVAS_WIDTH, DRAW_CANVAS_HEIGHT,
                         LV_COLOR_FORMAT_L8);
    lv_canvas_fill_bg(s_canvas, lv_color_white(), LV_OPA_COVER);
    lv_obj_add_flag(s_canvas, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_remove_flag(s_canvas, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_radius(s_canvas, 0, 0);
    lv_obj_add_event_cb(s_canvas, canvas_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(s_canvas, canvas_event_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(s_canvas, canvas_event_cb, LV_EVENT_RELEASED, NULL);
    lv_obj_add_event_cb(s_canvas, canvas_event_cb, LV_EVENT_PRESS_LOST, NULL);

    s_canvas_draw_buf = lv_canvas_get_draw_buf(s_canvas);

    s_hint_label = ui_create_label(page, "Canvas 400 x 160  |  line width 4 px",
                                   &lv_font_montserrat_16, UI_COLOR_MUTED);
    lv_obj_set_pos(s_hint_label, UI_CONTENT_LEFT, 220);

    s_coordinate_label = ui_create_label(page, "Coordinates hidden",
                                         &lv_font_montserrat_16, UI_COLOR_MUTED);
    lv_obj_align(s_coordinate_label, LV_ALIGN_BOTTOM_RIGHT, -96, -22);

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
    lv_obj_invalidate(s_canvas);
}

void screen_free_draw_set_line_width(uint8_t width)
{
    char text[64];

    s_line_width = (uint8_t)((width < 2U) ? 2U : ((width > 6U) ? 6U : width));
    if ((s_canvas == NULL) || (s_hint_label == NULL))
    {
        return;
    }

    lv_snprintf(text, sizeof(text), "Canvas 400 x 160  |  line width %u px",
                (unsigned)s_line_width);
    lv_label_set_text(s_hint_label, text);
}

void screen_free_draw_set_show_coordinates(bool show)
{
    s_show_coordinates = show;
    if (s_coordinate_label != NULL)
    {
        lv_label_set_text(s_coordinate_label, show ? "x:---  y:---" : "Coordinates hidden");
    }
}
