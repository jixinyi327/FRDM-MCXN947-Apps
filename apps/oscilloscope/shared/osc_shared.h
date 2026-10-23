/*
 * Copyright 2026
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef OSC_SHARED_H_
#define OSC_SHARED_H_

#include <stdbool.h>
#include <stdint.h>

#define OSC_SHARED_MAGIC        0x4F534331UL
#define OSC_SHARED_VERSION      1UL
#define OSC_SAMPLE_RATE_HZ      100000UL
#define OSC_SHARED_MEMORY_SIZE  0x2000UL
#define OSC_DMA_SAMPLES         1024U
#define OSC_WAVE_POINTS         512U
#define OSC_SPECTRUM_BINS       512U
#define OSC_ADC_CHANNEL         0U
#define OSC_INPUT_PIN_NAME      "J4.2 / A0"

enum
{
    OSC_FLAG_CORE1_READY = 1UL << 0U,
    OSC_FLAG_ADC_INIT_ERROR = 1UL << 1U,
    OSC_FLAG_DMA_OVERRUN = 1UL << 2U,
    OSC_FLAG_INPUT_CLIPPED = 1UL << 3U,
};

typedef struct
{
    uint32_t sample_rate_hz;
    uint32_t completed_buffers;
    uint32_t dma_overruns;
    uint32_t fft_cycles;
    uint32_t peak_frequency_hz;
    uint32_t flags;
    uint16_t minimum_raw;
    uint16_t maximum_raw;
    uint16_t waveform[OSC_WAVE_POINTS];
    uint16_t spectrum[OSC_SPECTRUM_BINS];
} osc_snapshot_t;

typedef struct
{
    volatile uint32_t sequence;
    uint32_t magic;
    uint32_t version;
    osc_snapshot_t data;
} osc_shared_state_t;

_Static_assert(
    sizeof(osc_shared_state_t) <= OSC_SHARED_MEMORY_SIZE,
    "oscilloscope shared state exceeds the reserved IPC region");

extern uint32_t __RPMSG_SH_MEM_START__[];

void osc_shared_initialize(void);
osc_shared_state_t *osc_shared_get(void);
void osc_shared_publish(const osc_snapshot_t *snapshot);
bool osc_shared_read(osc_snapshot_t *snapshot);

#endif /* OSC_SHARED_H_ */
