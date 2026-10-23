/*
 * Copyright 2026
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "osc_fft.h"

#include <stdlib.h>

#include "fsl_common.h"
#include "fsl_powerquad.h"

#define OSC_FFT_LOG2_N         10U
#define OSC_ADC_FULL_SCALE     4096U
#define OSC_ADC_MID_SCALE      2048
#define OSC_ADC_RESULT_MASK    0x0FFFU

static int16_t s_fft_input[OSC_DMA_SAMPLES];
static int16_t s_fft_output[OSC_DMA_SAMPLES * 2U];

static int16_t osc_fft_window_q15(uint32_t index)
{
    uint32_t distance = (2U * index > OSC_DMA_SAMPLES - 1U) ?
        (2U * index - (OSC_DMA_SAMPLES - 1U)) :
        ((OSC_DMA_SAMPLES - 1U) - 2U * index);
    return (int16_t)(32767U -
        ((distance * 32767U) / (OSC_DMA_SAMPLES - 1U)));
}

static int16_t osc_clip_q15(int32_t value)
{
    if (value > INT16_MAX)
    {
        return INT16_MAX;
    }
    if (value < INT16_MIN)
    {
        return INT16_MIN;
    }
    return (int16_t)value;
}

static uint32_t osc_abs_i32(int32_t value)
{
    return (value < 0) ? (uint32_t)(-value) : (uint32_t)value;
}

void osc_fft_init(void)
{
    pq_config_t config;

    PQ_Init(POWERQUAD);
    PQ_GetDefaultConfig(&config);
    config.inputAFormat = kPQ_16Bit;
    config.inputAPrescale = OSC_FFT_LOG2_N;
    config.inputBFormat = kPQ_16Bit;
    config.inputBPrescale = 0U;
    config.tmpFormat = kPQ_16Bit;
    config.tmpPrescale = 0U;
    config.outputFormat = kPQ_16Bit;
    config.outputPrescale = 2U;
    config.machineFormat = kPQ_32Bit;
    config.tmpBase = (uint32_t *)0xE0000000U;
    PQ_SetConfig(POWERQUAD, &config);

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void osc_fft_process(
    const uint32_t *raw_samples,
    uint32_t completed_buffers,
    uint32_t dma_overruns,
    osc_snapshot_t *snapshot)
{
    uint32_t cycle_start;
    uint32_t index;
    uint32_t maximum_magnitude = 0U;
    uint32_t peak_bin = 0U;
    uint16_t minimum = OSC_ADC_FULL_SCALE;
    uint16_t maximum = 0U;

    snapshot->sample_rate_hz = OSC_SAMPLE_RATE_HZ;
    snapshot->completed_buffers = completed_buffers;
    snapshot->dma_overruns = dma_overruns;
    snapshot->flags = OSC_FLAG_CORE1_READY;

    for (index = 0U; index < OSC_DMA_SAMPLES; index++)
    {
        uint16_t sample =
            (uint16_t)(raw_samples[index] & OSC_ADC_RESULT_MASK);
        int32_t centered = (int32_t)sample - OSC_ADC_MID_SCALE;
        int16_t window = osc_fft_window_q15(index);

        if (sample < minimum)
        {
            minimum = sample;
        }
        if (sample > maximum)
        {
            maximum = sample;
        }
        if ((sample <= 2U) || (sample >= OSC_ADC_FULL_SCALE - 3U))
        {
            snapshot->flags |= OSC_FLAG_INPUT_CLIPPED;
        }

        s_fft_input[index] = osc_clip_q15(
            (centered * 16 * (int32_t)window) / 32768);
    }

    snapshot->minimum_raw = minimum;
    snapshot->maximum_raw = maximum;
    for (index = 0U; index < OSC_WAVE_POINTS; index++)
    {
        snapshot->waveform[index] =
            (uint16_t)(raw_samples[index + (OSC_DMA_SAMPLES - OSC_WAVE_POINTS) / 2U] &
                       OSC_ADC_RESULT_MASK);
    }

    cycle_start = DWT->CYCCNT;
    PQ_TransformRFFT(
        POWERQUAD, OSC_DMA_SAMPLES, s_fft_input, s_fft_output);
    PQ_WaitDone(POWERQUAD);
    __DSB();
    snapshot->fft_cycles = DWT->CYCCNT - cycle_start;

    for (index = 1U; index < OSC_SPECTRUM_BINS; index++)
    {
        uint32_t magnitude =
            osc_abs_i32(s_fft_output[index * 2U]) +
            osc_abs_i32(s_fft_output[index * 2U + 1U]);

        if (magnitude > maximum_magnitude)
        {
            maximum_magnitude = magnitude;
            peak_bin = index;
        }
    }

    for (index = 0U; index < OSC_SPECTRUM_BINS; index++)
    {
        uint32_t magnitude = 0U;
        if (0U != index)
        {
            magnitude =
                osc_abs_i32(s_fft_output[index * 2U]) +
                osc_abs_i32(s_fft_output[index * 2U + 1U]);
        }
        snapshot->spectrum[index] = (0U == maximum_magnitude) ? 0U :
            (uint16_t)((magnitude * 1000U) / maximum_magnitude);
    }

    snapshot->peak_frequency_hz =
        (peak_bin * snapshot->sample_rate_hz) / OSC_DMA_SAMPLES;
    if (maximum_magnitude < 4U)
    {
        snapshot->peak_frequency_hz = 0U;
    }
}
