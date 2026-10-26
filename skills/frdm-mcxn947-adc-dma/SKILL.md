---
name: frdm-mcxn947-adc-dma
description: "Configure or review stable ADC+DMA sampling on FRDM-MCXN947 when the user specifies 12-bit or 16-bit resolution and a target sample rate in kS/s. Covers clock and trigger planning, continuous self-chained conversion, eDMA ping-pong buffers, result extraction, and runtime validation. Do not use for unrelated boards or standalone ADC theory."
---

# FRDM-MCXN947 ADC + DMA

Use this skill to implement or audit a deterministic ADC sampling path on
FRDM-MCXN947. The default hardware assumption is ADC0 channel A0, DMA0, and the
board's 48 MHz FRO_HF ADC clock.

## Required Input

If either value is missing, ask one concise question before editing:

```text
请提供 ADC 分辨率（12 或 16 位）和预期采样率。
采样率统一用 kS/s 表示，例如 2000k 表示 2 MS/s。
```

Accept `2000k`, `2000K`, `2000`, or `2M`; normalize all forms to samples per
second. Do not invent a default sample rate. Resolution and rate must be known
before choosing a trigger architecture.

## Planning

Run the planner with the user's normalized values:

```bash
python3 scripts/plan_adc_dma.py \
  --resolution 16 \
  --sample-rate-k 2000
```

The planner returns one of two stable architectures:

- `lptmr-periodic`: the requested rate is representable by the 12 MHz LPTMR and
  leaves at least one ADCK of conversion margin.
- `continuous-self-chain`: the requested rate is at the ADC throughput limit, so
  the ADC command links to itself and converts continuously after one software
  trigger.

If `feasible` is false, show the returned nearest supported rates and ask whether
the user accepts one of them. Do not silently relabel a different hardware rate
as the requested rate.

Before modifying an implementation, read
[references/adc-dma-implementation.md](references/adc-dma-implementation.md) and
apply the mode returned by the planner. For flashing or final verification, read
[references/acceptance-tests.md](references/acceptance-tests.md).

## Implementation Rules

1. Keep the ADC result pipeline deterministic: FIFO watermark drives eDMA, eDMA
   fills two non-cacheable aligned buffers, and only the completed buffer may be
   consumed.
2. For periodic rates, configure LPTMR0 from its 12 MHz source and connect its
   output through INPUTMUX to ADC0 trigger 0. Pass the planner's `timer_ticks`
   directly to `LPTMR_SetTimerPeriod()`.
3. For the ADC throughput limit, do not use an LPTMR trigger at the conversion
   period. Configure command chaining back to the same command, disable the
   hardware trigger wait, start DMA, then issue one software trigger.
4. Use the NXP-proven ADC clock, power, sample-time and calibration sequence.
   Do not change clocking or calibration and rate in the same experiment.
5. Use `ADC_RESFIFO_D_MASK`; shift by 3 for standard 12-bit conversion and by 0
   for high-resolution 16-bit conversion. Keep the logical full scale at 4096 or
   65536 respectively.
6. Record the actual ADC clock, conversion cycles, actual sample rate, trigger
   mode, FIFO count, buffer count and bytes per buffer in the implementation.
7. Preserve any existing UI/display ownership and shared-memory layout unless the
   user explicitly requests an integration redesign.

## Change and Verify

- Inspect the project and SDK worktree before editing; preserve unrelated local
  changes.
- Change one timing variable per experiment.
- Build the complete application, not only the ADC source file.
- Verify DMA completion count over a known interval and compare it with the
  planned buffer rate.
- Verify a known sine frequency through the application's frequency estimate or
  an exported sample block.
- Require zero FIFO overflow, zero DMA overrun and a stable long run before
  calling the configuration accepted.
- If hardware is connected, flash and request physical waveform confirmation.
  Compilation alone does not prove sampling correctness.

## Common Examples

```text
16-bit, 2000k  -> continuous self-chained ADC at the 2 MS/s limit
16-bit, 1000k  -> LPTMR period 12 ticks
16-bit, 500k   -> LPTMR period 24 ticks
12-bit, 2000k  -> LPTMR period 6 ticks with valid conversion margin
```

When the requested rate is not exactly representable, state the nearest actual
rate and its percentage error. Never claim an exact rate from a rounded timer
period.
