# Oscilloscope Performance Baseline

## Architecture

- UI core: `cm33_core0`
- Acquisition and DSP core: `cm33_core1`
- UI task period: 16 ms, nominal 62.5 FPS
- Display path: accepted LCD-PAR-S035 two-stripe TE/SmartDMA pipeline
- ADC rate: 2 MS/s
- ADC conversion: software-triggered, self-chained LPADC command
- ADC DMA: two linked TCDs, 3072 samples per buffer
- FFT: PowerQuad Q15 RFFT, 512 real samples
- Shared IPC: 8 KiB reserved `rpmsg_sh_mem`, sequence-protected snapshot

## Initial Link Result

```text
core0 m_text:       413,496 B / 767 KiB
core0 m_data:       237,168 B / 304 KiB
core0 core1 image:   43,348 B section / 43,340 B image
core1 m_text:        42,220 B / 51 KiB
core1 m_data:        44,424 B / 52 KiB
```

The core0 figure includes LVGL, two RGB565 display stripes, the I1 waveform canvas,
fonts and application state. Core1 includes the ADC buffers, FFT working arrays and
PowerQuad driver.

## Expected Cadence

- ADC buffer completion: every 1.536 ms, about 651 Hz;
- FFT job cadence: every fourth completed buffer, about 162.8 Hz;
- shared snapshot publish: after each queued FFT job, about 162.8 Hz;
- UI snapshot consumption and redraw: every 16 ms, nominal 62.5 FPS;
- display flush: unchanged accepted TE/SmartDMA path.

The 512-point FFT has a 3.90625 kHz raw bin spacing at 2 MS/s. When the 1536-point
waveform window contains at least two complete cycles, peak frequency is measured
from threshold crossings with linear interpolation; otherwise it falls back to
three-bin logarithmic interpolation. The spectrum array remains at the original
FFT resolution.

## Pending Measurements

- displayed FPS and frame time;
- worst-case core1 processing time under an applied signal;
- DMA overrun count under a 30-minute run;
- visual tearing and input latency on the physical panel.

## Initial Runtime Check

The dual-core image was flashed and reset on the connected FRDM-MCXN947.

```text
Serial:              core1=ready
Shared magic:        0x4F534331
Sample rate:         100000 Hz
Completed buffers:   5642
DMA overruns:        0
PowerQuad FFT time:  117 DWT cycles in the observed idle/no-signal sample
Sequence lock:       even and valid
```

This confirms that core1 starts from the embedded image and continues to publish
ADC/DMA/FFT snapshots into the shared region. No input signal was connected during
the runtime check, so waveform accuracy, frequency accuracy and panel appearance
remain pending physical acceptance.

## Continuous ADC Check

The periodic LPTMR trigger was replaced by a software-started, self-chained LPADC
command so that 16-bit conversions run continuously at the 2 MS/s ADC limit while
eDMA drains FIFO0.

Measured with a 10 kHz input:

```text
reported sample rate: 2,000,000 Hz
ADC min/max raw:      2480 / 62687
reported peak:        10,013 Hz
DMA overruns:         0
```

The shared-state reading confirms numerical acquisition and frequency output. The
on-screen visual check remains a separate physical acceptance item.

Build evidence does not constitute physical UI or analog-signal acceptance.
