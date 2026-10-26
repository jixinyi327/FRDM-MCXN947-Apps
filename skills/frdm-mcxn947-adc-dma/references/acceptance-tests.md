# ADC + DMA Acceptance Tests

## 1. Static Checks

- Planned resolution matches `conversionResolutionMode`.
- Result mask and shift produce 0..4095 or 0..65535.
- Planned actual sample rate is used everywhere; no rounded display value is
  reused as the sampling clock.
- LPTMR periodic mode leaves at least one ADCK conversion margin.
- Continuous mode chains the active command to itself and is software-started.
- DMA source is FIFO0 and both TCDs generate major-loop interrupts.
- Buffer alignment, cache attributes and ownership are explicit.

## 2. Build Gate

- Build the complete application and both cores.
- Inspect linker output for RAM headroom.
- Confirm the shared-memory region still fits.
- Check that no new compiler warnings or implicit conversions were introduced.

## 3. Runtime Throughput Gate

Measure completed DMA buffers over a known interval:

```text
observed_sample_rate = completed_buffers_delta
                     * samples_per_buffer
                     / elapsed_seconds
```

Accept only when the observed rate matches the planner's
`actual_sample_rate_hz` within measurement uncertainty. Watch for:

- FIFO overflow;
- DMA overrun;
- trigger exception;
- missed or non-monotonic completion counts;
- the consumer skipping buffers.

## 4. Signal Gate

Use a known frequency source:

1. Capture at least four complete cycles where possible.
2. Estimate frequency from interpolated threshold crossings.
3. Cross-check with the expected FFT bin and account for `Fs / N`.
4. Verify Vpp and midpoint against a separate meter or reference scope.

For 10 kHz at 2 MS/s, a stable waveform should contain about 200 samples per
cycle. A period near 160 samples indicates that the sampling clock or effective
trigger rate is wrong, not that FFT interpolation is missing.

## 5. Soak Gate

- Run at least 30 minutes for continuous capture.
- Require no FIFO overflow and no DMA overrun.
- Confirm the producer does not block the display owner.
- Confirm no gradual memory growth or sequence-lock failure.

Compilation, a single buffer or one successful flash does not prove stable
sampling. Record the actual mode, clock, rate, buffer size, observed rate and
errors in the project documentation.
