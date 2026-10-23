# UI Acceptance Tests

## 1. Build Gate

```bash
bash .../audit_ui_config.sh
bash .../build_ui.sh
```

Require:

- configuration audit passes or warnings are explicitly accepted;
- no compile warnings caused by the change;
- link succeeds;
- RAM/Flash headroom is recorded;
- patch fidelity is checked if an SDK overlay was used.

## 2. Flash Gate

- flash succeeds against the correct probe serial;
- target reaches the UI without reset loop;
- serial output remains healthy;
- no HardFault, BusFault, assert or repeated startup banner.

## 3. Visual Gate

Test on the real panel:

1. static page for 60 seconds;
2. slow drag 10 times;
3. fast drag 10 times;
4. drag for 2 minutes continuously;
5. page transition or animation;
6. stop motion and verify the image becomes stable immediately.

Record:

- whether tearing is absent, slight, moderate or severe;
- whether it is diagonal, horizontal or vertical;
- whether it persists when static;
- whether touch remains aligned;
- perceived input latency and stutter.

If the user cannot perform visual testing, mark the visual gate pending.

## 4. Runtime Gate

- 30-minute continuous UI stress;
- rapid touch and occasional power cycle;
- no memory growth;
- no spontaneous reset;
- no deadlock in TE or DMA completion;
- serial and touch remain responsive.

## 5. Performance Record

Capture:

| Metric | Value |
|---|---:|
| Configured UI period | |
| Measured average FPS | |
| Worst frame time | |
| Static RAM | |
| Flash text | |
| Buffer height | |
| Buffer bytes each | |
| TE mode | |
| Async or blocking flush | |
| CPU load | |
| Final visual result | |

## 6. Regression Rules

An accepted change must not:

- increase RAM without a documented need;
- remove the display-object guard from DMA callbacks;
- block one display frame per partial strip;
- reintroduce pixel-vs-byte buffer sizing;
- move LCD access to core1 without a full ownership redesign;
- claim fixed tearing without visual confirmation;
- claim 60 FPS from the configured period alone.

## 7. Experiment Record Template

Copy this record into the project documentation for every performance
experiment. Fill it only with evidence gathered in that run.

```markdown
# UI Performance Experiment: <short name>

- Date:
- Request or symptom:
- Hypothesis:
- Single changed variable:
- Baseline commit and accepted patch number:
- Candidate patch or working-tree diff:
- Hardware revision and panel:
- SDK, LVGL and compiler versions:

## Static evidence

- Audit result:
- Build result:
- m_text / m_data / static RAM:
- Compare result:

## Runtime evidence

- Configured UI period:
- Measured FPS and worst frame time:
- Serial faults or timeout messages:
- Stress duration:

## Physical visual evidence

- Static rendering:
- Slow drag, 10 repetitions:
- Fast drag, 10 repetitions:
- Two-minute continuous drag:
- Artifact shape and boundary:
- Input latency:
- Recovery after motion stops:

## Decision

- Keep or reject:
- Measured tradeoff:
- Follow-up or hardware limit:
```

Do not mark the physical visual fields as passing when the panel was not
observed. Build, serial and map-file evidence cannot prove visual correctness.

## 8. Final Decision

Keep the change only if the measured tradeoff matches the product requirement.
For fast-motion content, accept the ST7796 single-GRAM limit explicitly or plan
a hardware/display-controller change.
