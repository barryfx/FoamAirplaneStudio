# Servo Tray validation
Date: 2026-09-17

Windows Debug geometry test `servo_tray_geometry_tests` passed in 1.04 s.
It verifies 5 mm inward by 5 mm high support ledges, a separate tray fitted to
rectangular and tapered inner walls, zero positive-volume wall/tray overlap,
retained top-face geometry, cuts through supports, invalid placements and
cancellation. The revised inward dimension supersedes the original 3 mm request.

`servo_tray_tests` passed in 55.14 s. Real Qt mouse events cover drawing,
translation, corner resizing, deletion, draft save/reopen, version-14 persistence,
version-13 migration and malformed rectangle rejection. A pixel check confirms
the rectangle stays visible outside Servo Tray. The panel screenshot was visually
inspected. The full Fuselage worker produces two bodies and retains the tray top
faces; unchanged 3D navigation reuses the cache and the Wing revision stays zero.

Evidence (ignored local artifacts):
- `build/servo-tray-geometry-tests.log`
- `build/servo-tray-ui-tests.log`
- `build/servo-tray-final-build.log`
- `build/servo-tray-regression-build.log`
- `build/servo-tray-regression.log`
- `build/debug/servo-tray.png`

The Debug application rebuilt successfully. The affected Fuselage regressions
also passed: `fuselage_cut_tests` in 23.86 s and `fuselage_thicken_ui_tests` in
1.56 s. The rebuilt Debug application was launched.

No Wing-specific tests were run. GentleLady.foam remains unchanged. These tests
cover representative rectangular and tapered cavities, not every possible loft.
DXF/SVG export UI and manufacturing bevel/clearance choices remain future work.

## Dimension-entry revision (2026-09-17)

Replaced freehand drawing and corner resizing with Width/Height fields and
fixed-size dragging. Tests cover explicit mm/in input, bare project-unit input,
invalid input recovery, center-preserving size changes, fixed dimensions even
when dragging a corner, delete/recreate, save/reopen, overlay visibility and
full independent Fuselage generation/cache. The updated panel screenshot was
visually inspected: instructions, both fields and placement/delete controls fit.

Evidence: `build/servo-tray-dimensions-build.log`,
`build/servo-tray-dimensions-tests.log`, and
`build/debug/servo-tray-dimensions.png`. No Wing-specific tests were run.
The focused `servo_tray_tests` run passed in 52.35 s. Debug rebuilt successfully
and the rebuilt application was launched.
