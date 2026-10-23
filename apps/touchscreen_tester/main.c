/*
 * Copyright 2026
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "app.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "lvgl.h"
#include "lvgl_support.h"

#include "ui/app_ui.h"

#define UI_TICK_PERIOD_MS 1U
#define LVGL_TASK_PERIOD_TICK 16U

static volatile uint32_t s_tick;
static volatile bool s_ui_task_pending;

static void UI_SetupTick(void)
{
    if (0U != SysTick_Config(SystemCoreClock / (UI_TICK_PERIOD_MS * 1000U)))
    {
        PRINTF("Tick initialization failed\r\n");
        for (;;)
        {
        }
    }
}

void SysTick_Handler(void)
{
    s_tick++;
    lv_tick_inc(UI_TICK_PERIOD_MS);

    if ((s_tick % LVGL_TASK_PERIOD_TICK) == 0U)
    {
        s_ui_task_pending = true;
    }
}

int main(void)
{
    BOARD_InitHardware();
    UI_SetupTick();

    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    app_ui_init();

    PRINTF("Touchscreen tester started: 480x320, 16 ms UI cadence\r\n");

    for (;;)
    {
        while (!s_ui_task_pending)
        {
        }
        s_ui_task_pending = false;
        lv_timer_handler();
    }
}
