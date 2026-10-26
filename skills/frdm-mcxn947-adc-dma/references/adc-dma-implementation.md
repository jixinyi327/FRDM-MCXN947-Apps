# MCXN947 ADC + DMA Implementation

## Timing Model

The default FRDM-MCXN947 configuration uses FRO_HF at 48 MHz for ADC0.

| Resolution | ADCK cycles per conversion | Conversion time | Throughput limit |
|---:|---:|---:|---:|
| 12-bit standard | 19 | 395.833 ns | 2.526315 MS/s |
| 16-bit high | 24 | 500.000 ns | 2.000000 MS/s |

The cycle counts come from the MCX N reference manual and include the minimum
3.5-cycle sample time. A periodic hardware trigger is considered safe only when
its interval is at least one ADCK longer than the conversion time.

## Architecture Selection

### LPTMR periodic trigger

Use this mode when the planner returns `lptmr-periodic`.

```c
CLOCK_EnableClock(kCLOCK_InputMux0);
CLOCK_AttachClk(kFRO_HF_to_ADC0);
CLOCK_SetClkDiv(kCLOCK_DivAdc0Clk, 1U);

INPUTMUX_EnableSignal(
    INPUTMUX0, kINPUTMUX_Adc0FifoARequestToDma0Ch21Ena, true);
INPUTMUX_AttachSignal(
    INPUTMUX0, 0U, kINPUTMUX_Lptmr0ToAdc0Trigger);
```

Configure LPTMR0 from its 12 MHz source:

```c
lptmr_config_t timer;
LPTMR_GetDefaultConfig(&timer);
timer.prescalerClockSource = kLPTMR_PrescalerClock_0;
LPTMR_Init(LPTMR0, &timer);
/*
 * planner.timer_ticks already means "counter increments per trigger".
 * LPTMR_SetTimerPeriod() writes ticks - 1 to CMR.
 */
LPTMR_SetTimerPeriod(LPTMR0, planner_timer_ticks);
```

The ADC trigger must target command 1 and enable the hardware trigger:

```c
lpadc_conv_trigger_config_t trigger;
LPADC_GetDefaultConvTriggerConfig(&trigger);
trigger.targetCommandId = 1U;
trigger.enableHardwareTrigger = true;
LPADC_SetConvTriggerConfig(ADC0, 0U, &trigger);
```

### Continuous self-chain

Use this mode when the planner returns `continuous-self-chain`. It is required
when a periodic trigger leaves no conversion margin, including 16-bit 2 MS/s on
the default board clock.

```c
lpadc_conv_command_config_t command;
LPADC_GetDefaultConvCommandConfig(&command);
command.channelNumber = channel;
command.conversionResolutionMode =
    kLPADC_ConversionResolutionHigh; /* or Standard for 12-bit */
command.sampleTimeMode = kLPADC_SampleTimeADCK3;
command.chainedNextCommandNumber = 1U;
command.enableWaitTrigger = false;
LPADC_SetConvCommandConfig(ADC0, 1U, &command);

lpadc_conv_trigger_config_t trigger;
LPADC_GetDefaultConvTriggerConfig(&trigger);
trigger.targetCommandId = 1U;
trigger.enableHardwareTrigger = false;
LPADC_SetConvTriggerConfig(ADC0, 0U, &trigger);

LPADC_DoResetFIFO0(ADC0);
LPADC_EnableFIFO0WatermarkDMA(ADC0, true);
/* Start eDMA first, then kick command 1 once. */
LPADC_DoSoftwareTrigger(ADC0, 1UL);
```

Do not add an LPTMR trigger at the same 0.5 us interval. The command chain lets
the ADC pipeline conversions without competing with repeated trigger events.

## ADC Configuration

Use the NXP power and calibration sequence:

- highest LPADC power mode when the board clock permits it;
- `enableAnalogPreliminary = true`;
- reference source selected deliberately for the board;
- offset and gain calibration completed before starting DMA;
- minimum safe sample time for a low-impedance source;
- FIFO0 watermark DMA enabled only after calibration and FIFO reset.

Result extraction must match the selected resolution:

```c
#define RESULT_MASK   ADC_RESFIFO_D_MASK
#define RESULT_SHIFT  3U /* 0U for high-resolution 16-bit */

uint16_t sample =
    (uint16_t)((raw_fifo_word & RESULT_MASK) >> RESULT_SHIFT);
```

Logical full scale:

```text
12-bit: 4096
16-bit: 65536
```

## eDMA Ping-Pong

The ADC FIFO source is a 32-bit peripheral register. Store the raw FIFO word in
`uint32_t` buffers and convert to logical samples only when processing a completed
buffer.

Required properties:

- two DMA TCDs linked in a ring;
- one destination buffer per TCD;
- destination buffers in non-cacheable, at least 32-byte aligned memory;
- major-loop interrupt on both TCDs;
- completed-buffer index toggled only in the DMA interrupt;
- consumer reads only the buffer marked complete;
- no CPU writes to the buffer currently owned by DMA;
- overflow and overrun counters exposed in status data.

Do not point a second DMA transfer at a buffer still consumed by DSP. If the
consumer can fall behind, add a third slot or decimate/snapshot deliberately.

## Integration Rules

- Keep board clock setup on the core that owns the system clock configuration.
- Keep ADC, DMA and DSP ownership on one core unless an IPC contract explicitly
  defines otherwise.
- If snapshots are shared across cores, use a sequence or seqlock protocol.
- Do not copy more data into shared memory than the linker region can hold.
- Do not modify generated SDK files; put project overlays under the project or
  record them as numbered patches.
- Keep the planned and reported sample rate identical throughout timing,
  frequency, spectrum and UI calculations.

## Relevant NXP References

In the initialized MCUXpresso SDK repository:

```text
mcuxsdk/examples/driver_examples/lpadc/edma/lpadc_edma.c
mcuxsdk/examples/demo_apps/lpadc/lpadc_high_sample_rate/sample_signal/
mcuxsdk/drivers/lpadc/fsl_lpadc.c
mcuxsdk/drivers/lpadc/fsl_lpadc.h
mcuxsdk/drivers/lptmr/fsl_lptmr.h
mcuxsdk/drivers/edma/fsl_edma.h
```
