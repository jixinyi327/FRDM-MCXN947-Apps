/*
 * Copyright 2026
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef APP_UI_H_
#define APP_UI_H_

#include <stdbool.h>

#include "osc_shared.h"

void app_ui_init(void);
void app_ui_set_core_ready(bool ready);
void app_ui_update(const osc_snapshot_t *snapshot, bool valid);

#endif /* APP_UI_H_ */
