/*
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef SCREEN_FREE_DRAW_H
#define SCREEN_FREE_DRAW_H

#include "lvgl.h"

lv_obj_t *screen_free_draw_create(lv_obj_t *parent);
void screen_free_draw_clear(void);
void screen_free_draw_set_line_width(uint8_t width);
void screen_free_draw_set_show_coordinates(bool show);

#endif
