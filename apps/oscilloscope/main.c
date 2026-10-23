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
#include "mcmgr.h"

#include "osc_core1_boot.h"
#include "osc_shared.h"
#include "ui/app_ui.h"

#define UI_TICK_PERIOD_MS      1U
#define UI_TASK_PERIOD_TICKS   33U
#define CORE1_READY_TIMEOUT_MS 2000U

static volatile uint32_t s_tick;
static volatile bool s_ui_task_pending;
static volatile bool s_core1_ready;

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
    if (0U == (s_tick % UI_TASK_PERIOD_TICKS))
    {
        s_ui_task_pending = true;
    }
}

static void core1_ready_event(
    mcmgr_core_t core, uint16_t event_data, void *context)
{
    (void)core;
    (void)event_data;
    (void)context;
    s_core1_ready = true;
}

int main(void)
{
    osc_snapshot_t snapshot;
    bool valid;
    uint32_t started_at;

    (void)MCMGR_Init();
    BOARD_InitHardware();
    UI_SetupTick();

    osc_shared_initialize();
    lv_init();
    lv_port_disp_init();
    lv_port_indev_init();
    app_ui_init();

    (void)MCMGR_RegisterEvent(
        kMCMGR_RemoteApplicationEvent, core1_ready_event, NULL);
    if (!osc_core1_start())
    {
        PRINTF("Core1 start failed\r\n");
        app_ui_set_core_ready(false);
    }
    else
    {
        started_at = s_tick;
        while ((!s_core1_ready) &&
               ((s_tick - started_at) < CORE1_READY_TIMEOUT_MS))
        {
        }
        app_ui_set_core_ready(s_core1_ready);
        PRINTF(
            "Oscilloscope started: 100 kS/s, 1024-point PowerQuad FFT, core1=%s\r\n",
            s_core1_ready ? "ready" : "timeout");
    }

    for (;;)
    {
        while (!s_ui_task_pending)
        {
        }
        s_ui_task_pending = false;
        valid = osc_shared_read(&snapshot);
        app_ui_update(&snapshot, valid);
        lv_timer_handler();
    }
}
