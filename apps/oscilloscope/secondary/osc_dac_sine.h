/*
 * Copyright 2026
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef OSC_DAC_SINE_H_
#define OSC_DAC_SINE_H_

#include <stdbool.h>
#include <stdint.h>

#define OSC_DAC_SINE_FREQUENCY_HZ 10000U
#define OSC_DAC_SINE_OUTPUT_PIN   "J1.4 / P4_2"

bool osc_dac_sine_init(void);

#endif /* OSC_DAC_SINE_H_ */
