# Primary toolbar order — 2026-09-23

Changed the final three actions to Inspect, Weight and Balance, Export. Action
storage uses stable workspace IDs independently of visual order, preserving
readiness, fallback selection and saved project compatibility.

Built only `view_controls_tests`; the Debug application was not rebuilt or
restarted. `view_controls_tests` passed in 3.45 seconds, checking visual action
order, actual button dispatch for all three modes, correct persisted IDs,
availability and viewport-tab behavior using an empty cached compound. No Wing
or Fuselage generation ran. The resulting toolbar screenshot was inspected.
Existing Inspect/Export/project test expectations were updated for the new order.

Evidence: `build/debug/toolbar-order-build.log`, `toolbar-order-tests.log`, and
`view-controls.png`. The application executable retained its timestamp
(2026-09-23 14:32:32 local) and size (9,647,616 bytes) throughout validation.
