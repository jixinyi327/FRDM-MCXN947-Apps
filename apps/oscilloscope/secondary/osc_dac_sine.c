/*
 * Copyright 2026
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "osc_dac_sine.h"

#include "fsl_clock.h"
#include "fsl_common.h"
#include "fsl_ctimer.h"
#include "fsl_dac.h"
#include "fsl_port.h"
#include "fsl_spc.h"

#define OSC_DAC_BASE           DAC0
#define OSC_DAC_TIMER          CTIMER4
#define OSC_DAC_TIMER_IRQ      CTIMER4_IRQn
#define OSC_DAC_SAMPLE_RATE_HZ 100000U

static const uint32_t s_sine_lut[] = {
    2048, 2930, 3475, 3475, 2930, 2048, 1166, 621, 621, 1166,
};

static volatile uint32_t s_sine_index;

volatile uint32_t g_osc_dac_stage;

static void osc_dac_timer_callback(uint32_t flags)
{
    (void)flags;
    DAC_SetData(OSC_DAC_BASE, s_sine_lut[s_sine_index]);
    s_sine_index++;
    if (s_sine_index >= ARRAY_SIZE(s_sine_lut))
    {
        s_sine_index = 0U;
    }
}

static ctimer_callback_t s_timer_callbacks[] = {
    osc_dac_timer_callback,
};

static void osc_dac_configure_pin(void)
{
    const port_pin_config_t pin_config = {
        .pullSelect = kPORT_PullDisable,
        .pullValueSelect = kPORT_LowPullResistor,
        .slewRate = kPORT_FastSlewRate,
        .passiveFilterEnable = kPORT_PassiveFilterDisable,
        .openDrainEnable = kPORT_OpenDrainDisable,
        .driveStrength = kPORT_LowDriveStrength,
        .mux = kPORT_MuxAlt0,
        .inputBuffer = kPORT_InputBufferDisable,
        .invertInput = kPORT_InputNormal,
        .lockRegister = kPORT_UnlockRegister,
    };

    CLOCK_EnableClock(kCLOCK_Port4);
    PORT_SetPinConfig(PORT4, 2U, &pin_config);
}

bool osc_dac_sine_init(void)
{
    dac_config_t dac_config;
    ctimer_config_t timer_config;
    ctimer_match_config_t match_config;
    uint32_t timer_clock_hz;
    uint32_t sample_ticks;

    g_osc_dac_stage = 1U;
    CLOCK_AttachClk(kFRO_HF_to_DAC0);
    CLOCK_SetClkDiv(kCLOCK_DivDac0Clk, 1U);
    CLOCK_AttachClk(kFRO_HF_to_CTIMER4);
    CLOCK_SetClkDiv(kCLOCK_DivCtimer4Clk, 1U);
    SPC_EnableActiveModeAnalogModules(
        SPC0, kSPC_controlVref | kSPC_controlDac0);
    osc_dac_configure_pin();
    g_osc_dac_stage = 2U;

    DAC_GetDefaultConfig(&dac_config);
    dac_config.referenceVoltageSource =
        kDAC_ReferenceVoltageSourceAlt1;
    DAC_Init(OSC_DAC_BASE, &dac_config);
    DAC_SetData(OSC_DAC_BASE, s_sine_lut[0]);
    DAC_Enable(OSC_DAC_BASE, true);
    g_osc_dac_stage = 3U;

    timer_clock_hz = CLOCK_GetCTimerClkFreq(4U);
    sample_ticks = timer_clock_hz /
        (OSC_DAC_SINE_FREQUENCY_HZ * ARRAY_SIZE(s_sine_lut));
    if (sample_ticks > 1U)
    {
        sample_ticks--;
    }

    CTIMER_GetDefaultConfig(&timer_config);
    CTIMER_Init(OSC_DAC_TIMER, &timer_config);

    match_config.enableCounterReset = true;
    match_config.enableCounterStop = false;
    match_config.matchValue = sample_ticks;
    match_config.outControl = kCTIMER_Output_NoAction;
    match_config.outPinInitState = false;
    match_config.enableInterrupt = true;
    CTIMER_RegisterCallBack(
        OSC_DAC_TIMER, s_timer_callbacks, kCTIMER_SingleCallback);
    CTIMER_SetupMatch(
        OSC_DAC_TIMER, kCTIMER_Match_0, &match_config);
    EnableIRQ(OSC_DAC_TIMER_IRQ);
    CTIMER_StartTimer(OSC_DAC_TIMER);
    g_osc_dac_stage = 4U;
    return true;
}
