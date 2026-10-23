/*
 * Copyright 2026
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef OSC_ADC_H_
#define OSC_ADC_H_

#include <stdbool.h>
#include <stdint.h>

#include "osc_shared.h"

bool osc_adc_init(void);
bool osc_adc_take_completed(uint32_t *buffer_index);
uint32_t osc_adc_get_overruns(void);
const uint32_t *osc_adc_get_buffer(uint32_t buffer_index);

#endif /* OSC_ADC_H_ */
