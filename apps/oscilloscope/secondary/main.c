/*
 * Copyright 2026
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "app.h"
#include "mcmgr.h"

#include "osc_adc.h"
#include "osc_fft.h"
#include "osc_shared.h"

#define OSC_CORE_READY_EVENT 1U

int main(void)
{
    uint32_t startup_data = 0U;
    uint32_t buffer_index = 0U;
    uint32_t completed_buffers = 0U;
    osc_snapshot_t snapshot = {0};

    BOARD_InitHardware();
    (void)MCMGR_Init();
    while (kStatus_MCMGR_Success !=
           MCMGR_GetStartupData(kMCMGR_Core0, &startup_data))
    {
    }

    osc_fft_init();
    if (!osc_adc_init())
    {
        snapshot.sample_rate_hz = OSC_SAMPLE_RATE_HZ;
        snapshot.flags = OSC_FLAG_ADC_INIT_ERROR;
        osc_shared_publish(&snapshot);
        (void)MCMGR_TriggerEvent(
            kMCMGR_Core0, kMCMGR_RemoteApplicationEvent,
            OSC_CORE_READY_EVENT);
        for (;;)
        {
        }
    }

    (void)MCMGR_TriggerEvent(
        kMCMGR_Core0, kMCMGR_RemoteApplicationEvent,
        OSC_CORE_READY_EVENT);

    for (;;)
    {
        if (osc_adc_take_completed(&buffer_index))
        {
            completed_buffers++;
            osc_fft_process(
                osc_adc_get_buffer(buffer_index),
                completed_buffers,
                osc_adc_get_overruns(),
                &snapshot);
            osc_shared_publish(&snapshot);
        }
    }
}
