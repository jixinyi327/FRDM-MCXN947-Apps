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

static uint32_t osc_fft_energy(uint32_t index)
{
    int32_t real = s_fft_output[index * 2U];
    int32_t imaginary = s_fft_output[index * 2U + 1U];

    return (uint32_t)(real * real) +
        (uint32_t)(imaginary * imaginary);
}

static int32_t osc_log2_energy_q8(uint32_t magnitude_squared)
{
    uint32_t exponent;
    uint32_t mantissa_q15;
    uint32_t table_index;

    if (0U == magnitude_squared)
    {
        return 0;
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

    return (int32_t)(
        exponent * 256U + s_log2_fraction_q8[table_index]);
}

static uint16_t osc_adc_sample(const uint32_t *raw_samples, uint32_t index)
{
    return (uint16_t)(
        (raw_samples[index] & OSC_ADC_RESULT_MASK) >>
        OSC_ADC_RESULT_SHIFT);
}

static int32_t osc_dbfs_tenths(uint32_t magnitude_squared)
{
    int32_t log2_q8;
    int32_t delta_q8;
    int32_t dbfs_tenths;

    if (0U == magnitude_squared)
    {
        return OSC_DBFS_FLOOR;
    }

    log2_q8 = osc_log2_energy_q8(magnitude_squared);
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

static uint32_t osc_interpolated_peak_frequency_hz(
    uint32_t peak_bin, uint32_t sample_rate_hz)
{
    uint32_t previous_energy;
    uint32_t peak_energy;
    uint32_t next_energy;
    int32_t previous_log;
    int32_t peak_log;
    int32_t next_log;
    int32_t denominator;
    int32_t offset_q15;
    int64_t frequency_scaled;

    if ((0U == sample_rate_hz) ||
        (peak_bin < 1U) ||
        (peak_bin >= (OSC_FFT_SAMPLES / 2U)))
    {
        return (peak_bin * sample_rate_hz) / OSC_FFT_SAMPLES;
    }

    previous_energy = osc_fft_energy(peak_bin - 1U);
    peak_energy = osc_fft_energy(peak_bin);
    next_energy = osc_fft_energy(peak_bin + 1U);
    if ((0U == previous_energy) ||
        (0U == peak_energy) ||
        (0U == next_energy))
    {
        return (peak_bin * sample_rate_hz) / OSC_FFT_SAMPLES;
    }

    previous_log = osc_log2_energy_q8(previous_energy);
    peak_log = osc_log2_energy_q8(peak_energy);
    next_log = osc_log2_energy_q8(next_energy);
    denominator = previous_log - (2 * peak_log) + next_log;
    if (denominator >= 0)
    {
        return (peak_bin * sample_rate_hz) / OSC_FFT_SAMPLES;
    }

    /*
     * Parabolic interpolation of the logarithmic three-bin peak gives a
     * fractional offset in [-0.5, 0.5].  Q15 keeps the calculation integer.
     */
    offset_q15 = (int32_t)(
        (((int64_t)(previous_log - next_log)) * 16384) /
        denominator);
    if (offset_q15 < -16384)
    {
        offset_q15 = -16384;
    }
    else if (offset_q15 > 16384)
    {
        offset_q15 = 16384;
    }

    frequency_scaled =
        (((int64_t)peak_bin * 32768) + offset_q15) *
        sample_rate_hz;
    return (uint32_t)(
        (frequency_scaled + (OSC_FFT_SAMPLES * 32768) / 2) /
        (OSC_FFT_SAMPLES * 32768));
}

static uint32_t osc_waveform_frequency_hz(
    const osc_snapshot_t *snapshot)
{
    uint32_t span;
    uint32_t minimum;
    uint32_t maximum;
    uint32_t low_threshold;
    uint32_t midpoint;
    uint32_t previous_sample;
    uint32_t crossing_count = 0U;
    uint32_t first_crossing_q16 = 0U;
    uint32_t last_crossing_q16 = 0U;
    bool armed = false;
    uint32_t index;
    uint32_t duration_q16;
    uint64_t frequency;

    minimum = snapshot->minimum_raw;
    maximum = snapshot->maximum_raw;
    if ((0U == snapshot->sample_rate_hz) ||
        (maximum <= minimum))
    {
        return 0U;
    }

    span = maximum - minimum;
    if (span < 32U)
    {
        return 0U;
    }

    low_threshold = minimum + span / 4U;
    midpoint = minimum + span / 2U;
    previous_sample = snapshot->waveform[0];

    for (index = 1U; index < OSC_WAVE_POINTS; index++)
    {
        uint32_t sample = snapshot->waveform[index];

        if (!armed)
        {
            if (sample <= low_threshold)
            {
                armed = true;
            }
        }
        else if ((previous_sample < midpoint) &&
                 (sample >= midpoint))
        {
            uint32_t transition = sample - previous_sample;

            if (0U != transition)
            {
                uint32_t fraction_q16 = (uint32_t)(
                    (((uint64_t)(midpoint - previous_sample)) * 65536U) /
                    transition);
                uint32_t crossing_q16 =
                    (index - 1U) * 65536U + fraction_q16;

                if (0U == crossing_count)
                {
                    first_crossing_q16 = crossing_q16;
                }
                last_crossing_q16 = crossing_q16;
                crossing_count++;
            }
            armed = false;
        }
        previous_sample = sample;
    }

    if ((crossing_count < 3U) ||
        (last_crossing_q16 <= first_crossing_q16))
    {
        return 0U;
    }

    duration_q16 = last_crossing_q16 - first_crossing_q16;
    frequency =
        ((uint64_t)(crossing_count - 1U) *
         snapshot->sample_rate_hz * 65536U) /
        duration_q16;
    if (frequency > (snapshot->sample_rate_hz / 2U))
    {
        return 0U;
    }
    return (uint32_t)frequency;
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
        uint32_t magnitude_squared = osc_fft_energy(index);

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
            magnitude_squared = osc_fft_energy(index);
        }
        snapshot->spectrum[index] = (uint16_t)(
            (osc_dbfs_tenths(magnitude_squared) + 800) * 1000 / 800);
    }

    snapshot->peak_frequency_hz =
        osc_waveform_frequency_hz(snapshot);
    if (0U == snapshot->peak_frequency_hz)
    {
        snapshot->peak_frequency_hz =
            osc_interpolated_peak_frequency_hz(
                peak_bin, snapshot->sample_rate_hz);
    }
    if (maximum_magnitude_squared < 16U)
    {
        snapshot->peak_frequency_hz = 0U;
    }
}
