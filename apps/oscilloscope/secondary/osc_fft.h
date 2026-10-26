/*
 * Copyright 2026
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef OSC_FFT_H_
#define OSC_FFT_H_

#include <stdint.h>

#include "osc_shared.h"

void osc_fft_init(void);
void osc_fft_prepare(
    const uint32_t *raw_samples,
    uint32_t completed_buffers,
    uint32_t dma_overruns,
    osc_snapshot_t *snapshot,
    int16_t *fft_input);
void osc_fft_transform(
    osc_snapshot_t *snapshot, const int16_t *fft_input);

#endif /* OSC_FFT_H_ */
