# Oscilloscope Performance Baseline

## Architecture

- UI core: `cm33_core0`
- Acquisition and DSP core: `cm33_core1`
- UI task period: 33 ms, nominal 30 FPS
- Display path: accepted LCD-PAR-S035 two-stripe TE/SmartDMA pipeline
- ADC rate: 100 kS/s
- ADC DMA: two linked TCDs, 1024 samples per buffer
- FFT: PowerQuad Q15 RFFT, 1024 real samples
- Shared IPC: 8 KiB reserved `rpmsg_sh_mem`, sequence-protected snapshot

## Initial Link Result

```text
core0 m_text:       401,512 B / 767 KiB
core0 m_data:       226,160 B / 304 KiB
core0 core1 image:   30,836 B section / 30,820 B image
core1 m_text:        29,700 B / 51 KiB
core1 m_data:        18,136 B / 52 KiB
```

The core0 figure includes LVGL, two RGB565 display stripes, the I1 waveform canvas,
fonts and application state. Core1 includes the ADC buffers, FFT working arrays and
PowerQuad driver.

## Expected Cadence

- ADC half-buffer completion: every 10.24 ms, about 97.7 Hz;
- shared snapshot publish: on each completed half-buffer;
- UI snapshot consumption and redraw: every 33 ms, about 30 Hz;
- display flush: unchanged accepted TE/SmartDMA path.

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

Build evidence does not constitute physical UI or analog-signal acceptance.
