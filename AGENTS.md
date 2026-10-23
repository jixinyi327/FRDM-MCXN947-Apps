# Project Instructions

- For LVGL UI information architecture, tokens, styles, widget states, focus,
  touch targets, resource-aware design and UI code organization, use the
  `lvgl-ui` skill.
- For any LVGL, display, touch or UI performance work on this project, use the
  `frdm-mcxn947-lvgl-ui-performance` skill.
- Apply both skills when a UI change affects both presentation and the display
  pipeline: `lvgl-ui` governs the interface and
  `frdm-mcxn947-lvgl-ui-performance` protects board timing and refresh behavior.
- Preserve the verified display ownership, RAM budget, TE synchronization,
  partial buffering and dual-core boundaries documented by that skill.
- Do not edit generated LVGL or GUI Guider C/H files by hand.
- Build and flash alone do not prove visual correctness; request physical
  slow-drag and fast-drag confirmation from the user when the panel is involved.
