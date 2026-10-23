/*
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "screen_block_check.h"

#include <stdint.h>

#include "ui_common.h"

#define GRID_COLUMNS 12U
#define GRID_ROWS 7U
#define GRID_CELLS (GRID_COLUMNS * GRID_ROWS)
#define GRID_MARGIN 8
#define GRID_GAP 2

static lv_obj_t *s_grid;
static ui_header_t s_header;
static uint32_t s_touched[3];
static uint16_t s_touched_count;

static bool cell_is_touched(uint16_t index)
{
    return (s_touched[index >> 5U] & (1UL << (index & 31U))) != 0U;
}

static void set_cell_touched(uint16_t index)
{
    s_touched[index >> 5U] |= (1UL << (index & 31U));
}

static void grid_cell_size(int32_t *cell_width, int32_t *cell_height)
{
    *cell_width = (UI_CONTENT_WIDTH - (GRID_MARGIN * 2) -
                   ((int32_t)GRID_COLUMNS - 1) * GRID_GAP) / (int32_t)GRID_COLUMNS;
    *cell_height = (UI_CONTENT_HEIGHT - (GRID_MARGIN * 2) -
                    ((int32_t)GRID_ROWS - 1) * GRID_GAP) / (int32_t)GRID_ROWS;
}

static void update_status(void)
{
    char text[64];

    if (s_touched_count == GRID_CELLS)
    {
        ui_info_set(UI_PAGE_BLOCK_CHECK,
                    ui_tr(UI_TEXT_BLOCK_COVERAGE_COMPLETE));
        ui_info_set_detail(UI_PAGE_BLOCK_CHECK,
                           ui_tr(UI_TEXT_BLOCK_COVERAGE_COMPLETE_DETAIL));
    }
    else
    {
        ui_info_set(UI_PAGE_BLOCK_CHECK, ui_tr(UI_TEXT_BLOCK_COVERAGE));
        lv_snprintf(text, sizeof(text), ui_tr(UI_TEXT_BLOCK_COVERAGE_FORMAT),
                    (unsigned)s_touched_count, (unsigned)GRID_CELLS,
                    (unsigned)(GRID_CELLS - s_touched_count));
        ui_info_set_detail(UI_PAGE_BLOCK_CHECK, text);
    }
}

static void language_changed_cb(ui_language_t language)
{
    LV_UNUSED(language);
    lv_label_set_text(s_header.title, ui_tr(UI_TEXT_PAGE_BLOCK_CHECK_TITLE));
    lv_label_set_text(s_header.subtitle, ui_tr(UI_TEXT_PAGE_BLOCK_CHECK_SUBTITLE));
    update_status();
}

static void grid_draw_cb(lv_event_t *event)
{
    lv_obj_t *grid = lv_event_get_target_obj(event);
    lv_layer_t *layer = lv_event_get_layer(event);
    lv_area_t object_coords;
    lv_draw_rect_dsc_t rect_dsc;
    int32_t cell_width;
    int32_t cell_height;
    uint32_t row;
    uint32_t column;

    lv_obj_get_coords(grid, &object_coords);
    grid_cell_size(&cell_width, &cell_height);
    lv_draw_rect_dsc_init(&rect_dsc);
    rect_dsc.radius = 3;
    rect_dsc.border_width = 1;
    rect_dsc.border_color = UI_COLOR_BORDER;

    for (row = 0; row < GRID_ROWS; row++)
    {
        for (column = 0; column < GRID_COLUMNS; column++)
        {
            uint16_t index = (uint16_t)(row * GRID_COLUMNS + column);
            lv_area_t cell;

            cell.x1 = object_coords.x1 + GRID_MARGIN + (int32_t)column * (cell_width + GRID_GAP);
            cell.y1 = object_coords.y1 + GRID_MARGIN + (int32_t)row * (cell_height + GRID_GAP);
            cell.x2 = cell.x1 + cell_width - 1;
            cell.y2 = cell.y1 + cell_height - 1;

            rect_dsc.bg_opa = LV_OPA_COVER;
            rect_dsc.bg_color = cell_is_touched(index) ? UI_COLOR_ACCENT : UI_COLOR_SURFACE;
            rect_dsc.border_color = cell_is_touched(index) ? UI_COLOR_ACCENT : UI_COLOR_BORDER;
            lv_draw_rect(layer, &rect_dsc, &cell);
        }
    }
}

static void mark_at_point(lv_point_t point)
{
    lv_area_t object_coords;
    lv_area_t changed_area;
    int32_t cell_width;
    int32_t cell_height;
    int32_t local_x;
    int32_t local_y;
    int32_t column;
    int32_t row;
    uint16_t index;

    lv_obj_get_coords(s_grid, &object_coords);
    grid_cell_size(&cell_width, &cell_height);
    local_x = point.x - object_coords.x1 - GRID_MARGIN;
    local_y = point.y - object_coords.y1 - GRID_MARGIN;

    if ((local_x < 0) || (local_y < 0))
    {
        return;
    }

    column = local_x / (cell_width + GRID_GAP);
    row = local_y / (cell_height + GRID_GAP);
    if ((column >= (int32_t)GRID_COLUMNS) || (row >= (int32_t)GRID_ROWS))
    {
        return;
    }

    if (((local_x % (cell_width + GRID_GAP)) >= cell_width) ||
        ((local_y % (cell_height + GRID_GAP)) >= cell_height))
    {
        return;
    }

    index = (uint16_t)(row * (int32_t)GRID_COLUMNS + column);
    if (!cell_is_touched(index))
    {
        set_cell_touched(index);
        s_touched_count++;
        update_status();
    }

    changed_area.x1 = object_coords.x1 + GRID_MARGIN +
                      column * (cell_width + GRID_GAP) - 1;
    changed_area.y1 = object_coords.y1 + GRID_MARGIN +
                      row * (cell_height + GRID_GAP) - 1;
    changed_area.x2 = changed_area.x1 + cell_width + 1;
    changed_area.y2 = changed_area.y1 + cell_height + 1;
    lv_obj_invalidate_area(s_grid, &changed_area);
}

static void grid_event_cb(lv_event_t *event)
{
    lv_event_code_t code = lv_event_get_code(event);

    if ((code == LV_EVENT_PRESSED) || (code == LV_EVENT_PRESSING))
    {
        lv_point_t point;

        lv_indev_get_point(lv_indev_active(), &point);
        mark_at_point(point);
    }
    else if (code == LV_EVENT_DRAW_MAIN_END)
    {
        grid_draw_cb(event);
    }
}

lv_obj_t *screen_block_check_create(lv_obj_t *parent)
{
    lv_obj_t *page = ui_create_page(parent);

    s_header = ui_create_header(page, ui_tr(UI_TEXT_PAGE_BLOCK_CHECK_TITLE),
                                ui_tr(UI_TEXT_PAGE_BLOCK_CHECK_SUBTITLE));

    s_grid = lv_obj_create(page);
    lv_obj_set_size(s_grid, UI_CONTENT_WIDTH, UI_CONTENT_HEIGHT);
    lv_obj_set_pos(s_grid, UI_CONTENT_LEFT, UI_CONTENT_TOP);
    ui_style_apply_card_flat(s_grid);
    lv_obj_add_flag(s_grid, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(s_grid, grid_event_cb, LV_EVENT_PRESSED, NULL);
    lv_obj_add_event_cb(s_grid, grid_event_cb, LV_EVENT_PRESSING, NULL);
    lv_obj_add_event_cb(s_grid, grid_event_cb, LV_EVENT_DRAW_MAIN_END, NULL);

    (void)ui_i18n_subscribe(language_changed_cb);
    screen_block_check_reset();
    return page;
}

void screen_block_check_reset(void)
{
    s_touched[0] = 0U;
    s_touched[1] = 0U;
    s_touched[2] = 0U;
    s_touched_count = 0U;

    update_status();
    if (s_grid != NULL)
    {
        lv_obj_invalidate(s_grid);
    }
}
