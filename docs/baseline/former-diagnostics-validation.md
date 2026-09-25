# Former rail diagnostics — 2026-09-23

This records the initial diagnostic-only change. The subsequent reproduction,
numerical retry and shorter message are documented in `babybuzzard36-rail-fix.md`.

Inspected BabyBuzzard36.foam without modifying it. The saved reference requests
914.4 mm (36 inches), Specify Dimensions, four formers, and rotations -3/0/0/0.
The reported error originates in the residual-solid clearance check during rail
construction, before rail fusion or Assembly wing-seat cuts. It does not identify
which former in the old message; this investigation did not regenerate the
fuselage, so the offending former and underlying Boolean failure remain unverified.

The replacement message reports nose-to-tail former number, center X in mm/in,
thickness, angle, rail side and rejected-solid volume, explains the open cavity
requirement, and distinguishes possible position/rotation workarounds from proof
of invalid user dimensions. Rail dimensions remain fixed at 4 by 3 mm.

Debug application and `former_diagnostics_tests` built successfully. The focused
test passed, checking identity independent of input order, units, rotation,
front/back and left/right descriptions, and guidance. No generation tests ran.
Evidence: `build/debug/former-diagnostics-build.log` and
`build/debug/former-diagnostics-tests.log`.
