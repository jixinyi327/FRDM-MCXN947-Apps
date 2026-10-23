/*
 * Copyright 2026
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef UI_SCOPE_VIEW_H_
#define UI_SCOPE_VIEW_H_

#include <stdbool.h>

#include "lvgl.h"

#include "osc_shared.h"

typedef enum
{
    UI_SCOPE_MODE_WAVEFORM = 0,
    UI_SCOPE_MODE_SPECTRUM,
} ui_scope_mode_t;

void wd_scope_view_init(lv_obj_t *parent);
void wd_scope_view_set_mode(ui_scope_mode_t mode);
void wd_scope_view_render(const osc_snapshot_t *snapshot, bool valid);

#endif /* UI_SCOPE_VIEW_H_ */
