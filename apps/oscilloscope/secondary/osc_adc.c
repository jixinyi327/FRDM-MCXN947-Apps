/*
 * Copyright 2026
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#include "osc_adc.h"

#include "fsl_clock.h"
#include "fsl_common.h"
#include "fsl_edma.h"
#include "fsl_inputmux.h"
#include "fsl_lpadc.h"
#include "fsl_lptmr.h"
#include "fsl_spc.h"

#define OSC_ADC_BASE           ADC0
#define OSC_ADC_COMMAND_ID     1U
#define OSC_DMA_BASE           DMA0
#define OSC_DMA_CHANNEL        0U
#define OSC_DMA_IRQ            EDMA_0_CH0_IRQn
#define OSC_LPTMR_BASE         LPTMR0
#define OSC_TRIGGER_CLOCK_HZ   12000000U
#define OSC_TRIGGER_TICKS      6U

AT_NONCACHEABLE_SECTION_ALIGN(
    static uint32_t s_adc_buffers[2][OSC_DMA_SAMPLES], 32U);
AT_QUICKACCESS_SECTION_DATA_ALIGN(
    static edma_tcd_t s_dma_tcd[2], 32U);

static volatile uint32_t s_total_completed;
static volatile uint32_t s_consumed_completed;
static volatile uint32_t s_last_completed_buffer;
static volatile uint32_t s_overruns;
static osc_adc_complete_callback_t s_complete_callback;
static void *s_complete_callback_context;

static void osc_adc_configure_clock_and_trigger(void)
{
    CLOCK_EnableClock(kCLOCK_InputMux0);
    CLOCK_SetupClockCtrl(kCLOCK_FRO12MHZ_ENA);
    CLOCK_AttachClk(kFRO_HF_to_ADC0);
    CLOCK_SetClkDiv(kCLOCK_DivAdc0Clk, 1U);

    SPC_EnableActiveModeAnalogModules(SPC0, kSPC_controlVref);
    INPUTMUX_EnableSignal(
        INPUTMUX0, kINPUTMUX_Adc0FifoARequestToDma0Ch21Ena, true);
    INPUTMUX_AttachSignal(
        INPUTMUX0, 0U, kINPUTMUX_Lptmr0ToAdc0Trigger);
}

static void osc_adc_configure_converter(void)
{
    lpadc_config_t adc_config;
    lpadc_conv_command_config_t command_config;
    lpadc_conv_trigger_config_t trigger_config;

    LPADC_GetDefaultConfig(&adc_config);
#if defined(FSL_FEATURE_LPADC_HAS_CFG_PWRSEL) && \
    (FSL_FEATURE_LPADC_HAS_CFG_PWRSEL == 1U)
    adc_config.powerLevelMode = kLPADC_PowerLevelAlt4;
#endif
    adc_config.enableAnalogPreliminary = true;
    adc_config.powerUpDelay = 0x10U;
    adc_config.referenceVoltageSource = kLPADC_ReferenceVoltageAlt3;
    LPADC_Init(OSC_ADC_BASE, &adc_config);

#if defined(FSL_FEATURE_LPADC_HAS_CTRL_CALOFSMODE) && \
    FSL_FEATURE_LPADC_HAS_CTRL_CALOFSMODE
    LPADC_SetOffsetCalibrationMode(
        OSC_ADC_BASE, kLPADC_OffsetCalibration12bitMode);
#endif

#if defined(FSL_FEATURE_LPADC_HAS_CTRL_CALOFS) && \
    FSL_FEATURE_LPADC_HAS_CTRL_CALOFS
    LPADC_DoOffsetCalibration(OSC_ADC_BASE);
#endif

#if defined(FSL_FEATURE_LPADC_HAS_CTRL_CAL_REQ) && \
    FSL_FEATURE_LPADC_HAS_CTRL_CAL_REQ
    SDK_DelayAtLeastUs(1U, SystemCoreClock);
    LPADC_DoAutoCalibration(OSC_ADC_BASE);
#endif

    LPADC_GetDefaultConvCommandConfig(&command_config);
    command_config.channelNumber = OSC_ADC_CHANNEL;
    command_config.conversionResolutionMode =
        kLPADC_ConversionResolutionHigh;
    command_config.sampleTimeMode = kLPADC_SampleTimeADCK3;
    LPADC_SetConvCommandConfig(
        OSC_ADC_BASE, OSC_ADC_COMMAND_ID, &command_config);

    LPADC_GetDefaultConvTriggerConfig(&trigger_config);
    trigger_config.targetCommandId = OSC_ADC_COMMAND_ID;
    trigger_config.enableHardwareTrigger = true;
    LPADC_SetConvTriggerConfig(OSC_ADC_BASE, 0U, &trigger_config);
    LPADC_EnableFIFO0WatermarkDMA(OSC_ADC_BASE, true);
    LPADC_ClearStatusFlags(
        OSC_ADC_BASE,
        kLPADC_ResultFIFO0OverflowFlag | kLPADC_TriggerExceptionFlag);
}

static void osc_adc_configure_dma(void)
{
    edma_config_t dma_config;
    edma_channel_config_t channel_config;
    edma_transfer_config_t transfer_config;
    void *source = (void *)&OSC_ADC_BASE->RESFIFO[0];

    EDMA_GetDefaultConfig(&dma_config);
    EDMA_Init(OSC_DMA_BASE, &dma_config);

    channel_config.channelDataSignExtensionBitPosition = 0U;
    channel_config.channelPreemptionConfig.enableChannelPreemption = false;
    channel_config.channelPreemptionConfig.enablePreemptAbility = true;
    channel_config.channelRequestSource =
        kDma0RequestMuxAdc0FifoARequest;
    channel_config.protectionLevel = kEDMA_ChannelProtectionLevelUser;
#if !(defined(FSL_FEATURE_EDMA_HAS_NO_CH_SBR_SEC) && \
      FSL_FEATURE_EDMA_HAS_NO_CH_SBR_SEC)
    channel_config.securityLevel = kEDMA_ChannelSecurityLevelNonSecure;
#endif
    EDMA_InitChannel(OSC_DMA_BASE, OSC_DMA_CHANNEL, &channel_config);

    EDMA_PrepareTransfer(
        &transfer_config,
        source,
        sizeof(uint32_t),
        s_adc_buffers[0],
        sizeof(uint32_t),
        sizeof(uint32_t),
        sizeof(s_adc_buffers[0]),
        kEDMA_PeripheralToMemory);
    EDMA_TcdSetTransferConfig(
        &s_dma_tcd[0], &transfer_config, &s_dma_tcd[1]);

    EDMA_PrepareTransfer(
        &transfer_config,
        source,
        sizeof(uint32_t),
        s_adc_buffers[1],
        sizeof(uint32_t),
        sizeof(uint32_t),
        sizeof(s_adc_buffers[1]),
        kEDMA_PeripheralToMemory);
    EDMA_TcdSetTransferConfig(
        &s_dma_tcd[1], &transfer_config, &s_dma_tcd[0]);

    EDMA_TcdEnableInterrupts(&s_dma_tcd[0], kEDMA_MajorInterruptEnable);
    EDMA_TcdEnableInterrupts(&s_dma_tcd[1], kEDMA_MajorInterruptEnable);
    EDMA_InstallTCD(OSC_DMA_BASE, OSC_DMA_CHANNEL, &s_dma_tcd[0]);

    NVIC_SetPriority(OSC_DMA_IRQ, 3U);
    EnableIRQ(OSC_DMA_IRQ);
#if defined(FSL_FEATURE_EDMA_ASYNCHRO_REQUEST_CHANNEL_COUNT) && \
    FSL_FEATURE_EDMA_ASYNCHRO_REQUEST_CHANNEL_COUNT
    EDMA_EnableAsyncRequest(OSC_DMA_BASE, OSC_DMA_CHANNEL, true);
#endif
    EDMA_EnableChannelRequest(OSC_DMA_BASE, OSC_DMA_CHANNEL);
}

static void osc_adc_configure_timer(void)
{
    lptmr_config_t timer_config;

    LPTMR_GetDefaultConfig(&timer_config);
    timer_config.prescalerClockSource = kLPTMR_PrescalerClock_0;
    LPTMR_Init(OSC_LPTMR_BASE, &timer_config);
    LPTMR_SetTimerPeriod(OSC_LPTMR_BASE, OSC_TRIGGER_TICKS);
}

bool osc_adc_init(void)
{
    s_total_completed = 0U;
    s_consumed_completed = 0U;
    s_last_completed_buffer = 1U;
    s_overruns = 0U;

    osc_adc_configure_clock_and_trigger();
    osc_adc_configure_converter();
    osc_adc_configure_dma();
    osc_adc_configure_timer();
    return true;
}

void osc_adc_start(void)
{
    LPTMR_StartTimer(OSC_LPTMR_BASE);
}

void osc_adc_set_complete_callback(
    osc_adc_complete_callback_t callback, void *context)
{
    s_complete_callback = callback;
    s_complete_callback_context = context;
}

bool osc_adc_take_completed(uint32_t *buffer_index)
{
    uint32_t primask = DisableGlobalIRQ();
    uint32_t count = s_total_completed;

    if (count == s_consumed_completed)
    {
        EnableGlobalIRQ(primask);
        return false;
    }

    *buffer_index = s_last_completed_buffer;
    if ((count - s_consumed_completed) > 1U)
    {
        s_overruns += count - s_consumed_completed - 1U;
    }
    s_consumed_completed = count;
    EnableGlobalIRQ(primask);
    return true;
}

uint32_t osc_adc_get_overruns(void)
{
    return s_overruns;
}

const uint32_t *osc_adc_get_buffer(uint32_t buffer_index)
{
    return s_adc_buffers[buffer_index & 1U];
}

void EDMA_0_CH0_IRQHandler(void)
{
    if (0U != (EDMA_GetChannelStatusFlags(OSC_DMA_BASE, OSC_DMA_CHANNEL) &
               kEDMA_InterruptFlag))
    {
        EDMA_ClearChannelStatusFlags(
            OSC_DMA_BASE, OSC_DMA_CHANNEL, kEDMA_InterruptFlag);
        s_last_completed_buffer ^= 1U;
        __DMB();
        s_total_completed++;
        if (NULL != s_complete_callback)
        {
            s_complete_callback(s_complete_callback_context);
        }
    }
}
