# Full regression validation

Date: 2026-09-21. Windows Debug. The user explicitly requested all regression
tests, including component generation suites.

`cmake --build --preset windows-debug --parallel 4` succeeded. The full
`ctest --preset windows-debug` run exercised all 41 registered tests in 1493.90
seconds: 36 passed, four failed, and one was skipped.

The four failures were stale test fixture calibration assumptions after moving
project scaling from Fuselage Length to Wingspan. Export's synthetic former
fixture now uses a 45 mm wingspan over its mirrored 90-unit half-span to retain
0.25 mm per scene unit. Former and servo-tray editor fixtures use a 180 mm
wingspan to retain 1 mm per scene unit. Existing dimensional assertions were
preserved; application code was unchanged during this validation.

Rebuilt `assembly_tests`, `former_tests`, and `servo_tray_tests`. Reran
`export_tests`, `former_tests`, `former_defaults_tests`, `servo_tray_tests`, and
`fuselage_readiness_tests` (which shares the former fixture). All five passed in
73.62 seconds. Across the full run and these reruns, all 40 executable tests
passed. The existing `designrc_geometry_tests` placeholder returns skip code 77
and provides no coverage; the actual component geometry suites ran and passed.

Local logs: `build/full-regression-build.log`, `build/full-regression-tests.log`,
`build/export-fixture-build.log`, `build/editor-fixture-build.log`, and
`build/full-regression-recheck.log`. Linux and macOS were not exercised.
