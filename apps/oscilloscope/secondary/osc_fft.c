/*
 * Copyright 2026
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "osc_fft.h"

#include <stdlib.h>

#include "fsl_common.h"
#include "fsl_powerquad.h"

#define OSC_FFT_LOG2_N         9U
#define OSC_ADC_FULL_SCALE     65536U
#define OSC_ADC_MID_SCALE      32768
#define OSC_ADC_RESULT_MASK    0xFFFFU
#define OSC_ADC_RESULT_SHIFT   0U
#define OSC_FFT_FULL_SCALE_MAG 8192U
#define OSC_DBFS_FLOOR         (-800)

static int16_t s_fft_output[OSC_FFT_SAMPLES * 2U];

static const uint16_t s_log2_fraction_q8[16] = {
    0U, 22U, 44U, 63U, 82U, 100U, 118U, 134U,
    150U, 165U, 179U, 193U, 207U, 220U, 232U, 244U,
};

static int16_t osc_fft_window_q15(uint32_t index)
{
    uint32_t distance = (2U * index > OSC_FFT_SAMPLES - 1U) ?
        (2U * index - (OSC_FFT_SAMPLES - 1U)) :
        ((OSC_FFT_SAMPLES - 1U) - 2U * index);
    return (int16_t)(32767U -
        ((distance * 32767U) / (OSC_FFT_SAMPLES - 1U)));
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

static uint16_t osc_adc_sample(const uint32_t *raw_samples, uint32_t index)
{
    return (uint16_t)(
        (raw_samples[index] & OSC_ADC_RESULT_MASK) >>
        OSC_ADC_RESULT_SHIFT);
}

static int32_t osc_dbfs_tenths(uint32_t magnitude_squared)
{
    uint32_t exponent;
    uint32_t mantissa_q15;
    uint32_t table_index;
    int32_t log2_q8;
    int32_t delta_q8;
    int32_t dbfs_tenths;

    if (0U == magnitude_squared)
    {
        return OSC_DBFS_FLOOR;
    }

    exponent = 31U - (uint32_t)__CLZ(magnitude_squared);
    if (exponent >= 15U)
    {
        mantissa_q15 = magnitude_squared >> (exponent - 15U);
    }
    else
    {
        mantissa_q15 = magnitude_squared << (15U - exponent);
    }
    table_index = (mantissa_q15 - 32768U) >> 11U;
    if (table_index > 15U)
    {
        table_index = 15U;
    }

    log2_q8 =
        (int32_t)(exponent * 256U + s_log2_fraction_q8[table_index]);
    delta_q8 = log2_q8 -
        (int32_t)(2U * 13U * 256U);
    dbfs_tenths = (delta_q8 * 60206) / 256000;
    if (dbfs_tenths < OSC_DBFS_FLOOR)
    {
        dbfs_tenths = OSC_DBFS_FLOOR;
    }
    if (dbfs_tenths > 0)
    {
        dbfs_tenths = 0;
    }
    return dbfs_tenths;
}

void osc_fft_init(void)
{
    pq_config_t config;

    PQ_Init(POWERQUAD);
    PQ_GetDefaultConfig(&config);
    config.inputAFormat = kPQ_16Bit;
    config.inputAPrescale = 0U;
    config.inputBFormat = kPQ_16Bit;
    config.inputBPrescale = 0U;
    config.tmpFormat = kPQ_32Bit;
    config.tmpPrescale = 0U;
    config.outputFormat = kPQ_16Bit;
    config.outputPrescale = 0U;
    config.machineFormat = kPQ_32Bit;
    config.tmpBase = (uint32_t *)0xE0000000U;
    PQ_SetConfig(POWERQUAD, &config);

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;
}

void osc_fft_prepare(
    const uint32_t *raw_samples,
    uint32_t completed_buffers,
    uint32_t dma_overruns,
    osc_snapshot_t *snapshot,
    int16_t *fft_input)
{
    uint32_t index;
    uint32_t trigger_start;
    uint16_t minimum = OSC_ADC_FULL_SCALE - 1U;
    uint16_t maximum = 0U;
    uint16_t low_threshold;
    uint16_t high_threshold;
    bool trigger_armed = false;

    snapshot->sample_rate_hz = OSC_SAMPLE_RATE_HZ;
    snapshot->completed_buffers = completed_buffers;
    snapshot->dma_overruns = dma_overruns;
    snapshot->flags = OSC_FLAG_CORE1_READY;
    if (0U != dma_overruns)
    {
        snapshot->flags |= OSC_FLAG_DMA_OVERRUN;
    }

    for (index = 0U; index < OSC_DMA_SAMPLES; index++)
    {
        uint16_t sample = osc_adc_sample(raw_samples, index);

        if (sample < minimum)
        {
            minimum = sample;
        }
        if (sample > maximum)
        {
            maximum = sample;
        }
        if ((sample <= 8U) || (sample >= OSC_ADC_FULL_SCALE - 9U))
        {
            snapshot->flags |= OSC_FLAG_INPUT_CLIPPED;
        }
    }

    snapshot->minimum_raw = minimum;
    snapshot->maximum_raw = maximum;
    trigger_start = OSC_DMA_SAMPLES - OSC_WAVE_POINTS;
    if ((uint32_t)(maximum - minimum) >= 32U)
    {
        uint32_t span = (uint32_t)(maximum - minimum);

        low_threshold =
            (uint16_t)(minimum + span / 4U);
        high_threshold =
            (uint16_t)(maximum - span / 4U);
        for (index = 1U;
             index <= OSC_DMA_SAMPLES - OSC_WAVE_POINTS;
             index++)
        {
            uint16_t sample = osc_adc_sample(raw_samples, index);

            if (sample <= low_threshold)
            {
                trigger_armed = true;
            }
            else if (trigger_armed && (sample >= high_threshold))
            {
                trigger_start = index;
                break;
            }
        }
    }

    for (index = 0U; index < OSC_FFT_SAMPLES; index++)
    {
        int32_t centered =
            (int32_t)osc_adc_sample(
                raw_samples, trigger_start + index) -
            OSC_ADC_MID_SCALE;
        int16_t window = osc_fft_window_q15(index);

        fft_input[index] = osc_clip_q15(
            (centered * (int32_t)window) / 32768);
    }

    for (index = 0U; index < OSC_WAVE_POINTS; index++)
    {
        snapshot->waveform[index] =
            osc_adc_sample(raw_samples, trigger_start + index);
    }
}

void osc_fft_transform(
    osc_snapshot_t *snapshot, const int16_t *fft_input)
{
    uint32_t cycle_start;
    uint32_t index;
    uint32_t maximum_magnitude_squared = 0U;
    uint32_t peak_bin = 0U;

    cycle_start = DWT->CYCCNT;
    PQ_TransformRFFT(
        POWERQUAD, OSC_FFT_SAMPLES, (void *)fft_input, s_fft_output);
    PQ_WaitDone(POWERQUAD);
    __DSB();
    snapshot->fft_cycles = DWT->CYCCNT - cycle_start;

    for (index = 1U; index < OSC_SPECTRUM_BINS; index++)
    {
        int32_t real = s_fft_output[index * 2U];
        int32_t imaginary = s_fft_output[index * 2U + 1U];
        uint32_t magnitude_squared =
            (uint32_t)(real * real) + (uint32_t)(imaginary * imaginary);

        if (magnitude_squared > maximum_magnitude_squared)
        {
            maximum_magnitude_squared = magnitude_squared;
            peak_bin = index;
        }
    }

    for (index = 0U; index < OSC_SPECTRUM_BINS; index++)
    {
        uint32_t magnitude_squared = 0U;
        if (0U != index)
        {
            int32_t real = s_fft_output[index * 2U];
            int32_t imaginary = s_fft_output[index * 2U + 1U];
            magnitude_squared =
                (uint32_t)(real * real) +
                (uint32_t)(imaginary * imaginary);
        }
        snapshot->spectrum[index] = (uint16_t)(
            (osc_dbfs_tenths(magnitude_squared) + 800) * 1000 / 800);
    }

    snapshot->peak_frequency_hz =
        (peak_bin * snapshot->sample_rate_hz) / OSC_FFT_SAMPLES;
    if (maximum_magnitude_squared < 16U)
    {
        snapshot->peak_frequency_hz = 0U;
    }
}
