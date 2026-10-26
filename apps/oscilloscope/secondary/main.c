/*
 * Copyright 2026
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

#include <string.h>

#include "mcmgr.h"

#include "osc_adc.h"
#include "osc_dac_sine.h"
#include "osc_fft.h"
#include "osc_shared.h"

#define OSC_CORE_READY_EVENT 1U
#define OSC_FFT_DECIMATION   4U

#define OSC_CAPTURE_TASK_PRIORITY 4U
#define OSC_FFT_TASK_PRIORITY     3U
#define OSC_CAPTURE_STACK_WORDS   384U
#define OSC_FFT_STACK_WORDS       512U

typedef struct
{
    osc_snapshot_t snapshot;
    int16_t fft_input[OSC_FFT_SAMPLES];
} osc_dsp_job_t;

volatile uint32_t g_osc_core1_stage;

static osc_dsp_job_t s_jobs[2];
static volatile bool s_job_free[2] = {true, true};
static StaticQueue_t s_job_queue;
static uint8_t s_job_queue_storage[2];
static QueueHandle_t s_job_queue_handle;

static StaticTask_t s_capture_task_tcb;
static StackType_t s_capture_task_stack[OSC_CAPTURE_STACK_WORDS];
static StaticTask_t s_fft_task_tcb;
static StackType_t s_fft_task_stack[OSC_FFT_STACK_WORDS];
static StaticTask_t s_idle_task_tcb;
static StackType_t s_idle_task_stack[configMINIMAL_STACK_SIZE];
static TaskHandle_t s_capture_task_handle;

static uint32_t s_capture_sequence;

void vApplicationGetIdleTaskMemory(
    StaticTask_t **idle_task_tcb,
    StackType_t **idle_task_stack,
    uint32_t *idle_task_stack_size)
{
    *idle_task_tcb = &s_idle_task_tcb;
    *idle_task_stack = s_idle_task_stack;
    *idle_task_stack_size = configMINIMAL_STACK_SIZE;
}

static void osc_adc_complete_isr(void *context)
{
    BaseType_t task_woken = pdFALSE;

    (void)context;
    if (NULL != s_capture_task_handle)
    {
        vTaskNotifyGiveFromISR(s_capture_task_handle, &task_woken);
        portYIELD_FROM_ISR(task_woken);
    }
}

static void osc_capture_task(void *argument)
{
    uint32_t buffer_index;

    (void)argument;
    g_osc_core1_stage = 6U;
    osc_adc_start();

    for (;;)
    {
        (void)ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        while (osc_adc_take_completed(&buffer_index))
        {
            uint32_t slot;
            bool queued = false;

            s_capture_sequence++;
            if (0U != (s_capture_sequence % OSC_FFT_DECIMATION))
            {
                continue;
            }

            for (slot = 0U; slot < 2U; slot++)
            {
                if (s_job_free[slot])
                {
                    s_job_free[slot] = false;
                    osc_fft_prepare(
                        osc_adc_get_buffer(buffer_index),
                        s_capture_sequence,
                        osc_adc_get_overruns(),
                        &s_jobs[slot].snapshot,
                        s_jobs[slot].fft_input);
                    if (pdPASS == xQueueSend(
                            s_job_queue_handle, &slot, 0U))
                    {
                        queued = true;
                    }
                    else
                    {
                        s_job_free[slot] = true;
                    }
                    break;
                }
            }
            (void)queued;
        }
    }
}

static void osc_fft_task(void *argument)
{
    uint8_t slot;

    (void)argument;
    g_osc_core1_stage = 7U;
    for (;;)
    {
        if (pdPASS == xQueueReceive(
                s_job_queue_handle, &slot, portMAX_DELAY))
        {
            osc_fft_transform(
                &s_jobs[slot].snapshot, s_jobs[slot].fft_input);
            osc_shared_publish(&s_jobs[slot].snapshot);
            s_job_free[slot] = true;
            g_osc_core1_stage = 8U;
        }
    }
}

int main(void)
{
    uint32_t startup_data = 0U;
    BaseType_t task_created;

    g_osc_core1_stage = 1U;
    (void)MCMGR_Init();
    g_osc_core1_stage = 2U;
    while (kStatus_MCMGR_Success !=
           MCMGR_GetStartupData(kMCMGR_Core0, &startup_data))
    {
    }

    memset(s_jobs, 0, sizeof(s_jobs));
    osc_fft_init();
    g_osc_core1_stage = 3U;
    if (!osc_adc_init())
    {
        return 0;
    }
    g_osc_core1_stage = 4U;
    (void)osc_dac_sine_init();

    s_job_queue_handle = xQueueCreateStatic(
        2U,
        sizeof(uint8_t),
        s_job_queue_storage,
        &s_job_queue);
    if (NULL == s_job_queue_handle)
    {
        for (;;)
        {
        }
    }

    s_capture_task_handle = xTaskCreateStatic(
        osc_capture_task,
        "adc",
        OSC_CAPTURE_STACK_WORDS,
        NULL,
        OSC_CAPTURE_TASK_PRIORITY,
        s_capture_task_stack,
        &s_capture_task_tcb);
    task_created = (NULL != s_capture_task_handle) ? pdPASS : pdFAIL;
    if (pdPASS == task_created)
    {
        task_created = (NULL != xTaskCreateStatic(
            osc_fft_task,
            "fft",
            OSC_FFT_STACK_WORDS,
            NULL,
            OSC_FFT_TASK_PRIORITY,
            s_fft_task_stack,
            &s_fft_task_tcb)) ? pdPASS : pdFAIL;
    }
    if (pdPASS != task_created)
    {
        for (;;)
        {
        }
    }

    osc_adc_set_complete_callback(osc_adc_complete_isr, NULL);
    (void)MCMGR_TriggerEvent(
        kMCMGR_Core0, kMCMGR_RemoteApplicationEvent,
        OSC_CORE_READY_EVENT);
    g_osc_core1_stage = 5U;

    vTaskStartScheduler();
    for (;;)
    {
        g_osc_core1_stage = 0xFFU;
    }
}
