# Fuselage profile and solid validation
Date: 2026-09-16

Debug application rebuilt successfully with the Windows Debug preset. Final
focused CTest run: 4/4 passed in 57.70 seconds:

- fuselage_outline_tests: 2.21 s
- fuselage_profile_tests: 54.63 s
- reference_tests: 0.22 s
- designrc_gui_tests: 0.63 s

No Wing-specific test suites were run, as requested. Build output is in
`build/fuselage-profiles-build-final.log`; test output is in
`build/fuselage-profiles-tests.log` (ignored local artifacts).

The profile test exercises actual mouse drawing of Line and Spline sections,
station highlighting, toolbar gating, selecting/reopening sketches, station moves
without reassignment, Delete Profile, Save/Open, unfinished spline restoration,
version-10 migration and invalid slot rejection. The real Fuselage background job
completes and increments only its model revision; repeated 3D entry reuses the
result. This run does not claim a Wing build regression or benchmark.

Geometry fixtures verify a 500 mm body from differently positioned/scaled Top and
Side rectangles: width 100 mm, height 150 mm, nose at X=0, centered Y/Z spans and
expected volume. A single-profile pointed-end solid also validates in actual-scale
mode. Pre-cancelled generation throws ProcessingCancelled.

Inspected `build/debug/fuselage-edit-profiles.png`: instructions, selected orange
station, active profile sketch, Line/Spline/Delete Profile and unlocked downstream
actions are visible. Inspected the native OCCT capture
`build/debug/fuselage-solid.png`: a closed solid transitions between rectangular
and rounded sections. Qt widget-only screenshots omit the native render surface;
the inspected 3D capture uses the screen/window capture API.

Source/deployed HTML help hashes match. `git diff --check` passes. The rebuilt
Debug application was launched after validation. Sampling, end caps and remaining
editor scope are documented in architecture/fuselage-profiles.md and ADR-0021.
