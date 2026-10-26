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
#include "ui/ui_scope_view.h"
#include "ui/ui_theme.h"

#define UI_TICK_PERIOD_MS      1U
#define UI_TASK_PERIOD_TICKS   16U
#define CORE1_READY_TIMEOUT_MS 2000U
#define OSC_MINIMAL_UI_DIAGNOSTIC 0
#define OSC_DIAGNOSTIC_UI_STAGE 3U
#define OSC_SKIP_CORE1_DIAGNOSTIC 0

static volatile uint32_t s_tick;
static volatile bool s_ui_task_pending;
static volatile bool s_core1_ready;
static bool s_ui_loop_started;
static volatile uint32_t s_boot_stage;
static osc_snapshot_t s_snapshot;

#if OSC_MINIMAL_UI_DIAGNOSTIC
static void UI_DiagnosticInit(void)
{
    lv_obj_t *screen = lv_screen_active();
    lv_obj_t *label;

#if OSC_DIAGNOSTIC_UI_STAGE >= 2U
    ui_theme_init();
    ui_theme_set_language(UI_LANGUAGE_ZH);
    ui_style_apply_screen(screen);
#endif

    label = lv_label_create(screen);
#if OSC_DIAGNOSTIC_UI_STAGE >= 2U
    lv_label_set_text(label, "示波器");
    lv_obj_set_style_text_font(label, ui_theme_title_font(), LV_PART_MAIN);
#else
    lv_label_set_text(label, "OSC READY");
#endif
    lv_obj_center(label);
#if OSC_DIAGNOSTIC_UI_STAGE >= 3U
    wd_scope_view_init(screen);
#endif
}
#endif

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
#if !OSC_MINIMAL_UI_DIAGNOSTIC
    bool valid;
#endif
    uint32_t started_at;

    (void)MCMGR_Init();
    s_boot_stage = 1U;
    BOARD_InitHardware();
    s_boot_stage = 2U;
    UI_SetupTick();

    osc_shared_initialize();
    s_boot_stage = 3U;
    lv_init();
    s_boot_stage = 4U;
    PRINTF("Oscilloscope init: LVGL\r\n");
    lv_port_disp_init();
    s_boot_stage = 5U;
    PRINTF("Oscilloscope init: display\r\n");
    lv_port_indev_init();
    s_boot_stage = 6U;
#if OSC_MINIMAL_UI_DIAGNOSTIC
    UI_DiagnosticInit();
#else
    app_ui_init();
#endif
    s_boot_stage = 7U;
    PRINTF("Oscilloscope init: UI\r\n");

#if !OSC_SKIP_CORE1_DIAGNOSTIC
    (void)MCMGR_RegisterEvent(
        kMCMGR_RemoteApplicationEvent, core1_ready_event, NULL);
    s_boot_stage = 9U;
    if (!osc_core1_start())
    {
        s_boot_stage = 10U;
        PRINTF("Core1 start failed\r\n");
    }
    else
    {
        s_boot_stage = 11U;
        started_at = s_tick;
        while ((!s_core1_ready) &&
               ((s_tick - started_at) < CORE1_READY_TIMEOUT_MS))
        {
        }
    }
#else
    s_core1_ready = false;
#endif

    s_boot_stage = 12U;
    PRINTF("Oscilloscope init: core1 handshake complete\r\n");
#if !OSC_MINIMAL_UI_DIAGNOSTIC
    app_ui_set_core_ready(s_core1_ready);
#endif
    s_boot_stage = 13U;
    PRINTF(
        "Oscilloscope started: 2 MS/s, 1024-point PowerQuad FFT, core1=%s\r\n",
        s_core1_ready ? "ready" : "timeout");

    for (;;)
    {
        while (!s_ui_task_pending)
        {
        }
        s_ui_task_pending = false;
        s_boot_stage = 14U;
#if !OSC_MINIMAL_UI_DIAGNOSTIC
        valid = osc_shared_read(&s_snapshot);
        app_ui_update(&s_snapshot, valid);
#endif
        s_boot_stage = 15U;
        lv_timer_handler();
        s_boot_stage = 16U;
        if (!s_ui_loop_started)
        {
            s_ui_loop_started = true;
            PRINTF("Oscilloscope UI loop active\r\n");
        }
    }
}
