# Wingspan-only Reference validation

Date: 2026-09-21. Windows Debug.

- Built `designrc` and `reference_tests` successfully with the windows-debug preset.
- Ran only `ctest --preset windows-debug -R '^reference_tests$' --timeout 120`:
  1/1 passed in 0.46 seconds (0.52 seconds total).
- No geometry or component-generation tests were run, as requested.
- Reference tests verify the exact User Reference Image Scale label, absence of
  the Fuselage Length widget, Wingspan-only readiness, invalid/cleared values,
  unit-preserving edits, reset, and image metadata behavior.
- Drawing-calibration checks verify manual/actual scale, changes to Wingspan,
  and invariance under drawing translation and rotation without building solids.
- Inspected `build/reference-scale-panel.png`: the label fits, and Specify
  Dimensions displays only Project Units and Wingspan.
- `git diff --check` passed.

Legacy files remain readable; their old independent fuselage scale is ignored.
Solid generation with the new shared scale was not exercised in this validation.
See ADR-0040 for the scaling and compatibility decision.
