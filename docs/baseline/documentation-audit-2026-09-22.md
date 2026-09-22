# Documentation audit — September 22, 2026

Reviewed current README, documentation guide, requirements, GUI design,
architecture, project format, ADRs and embedded help against the accumulated
Weight and Balance branch changes. Dated validation reports, earlier ADR decisions
and imported DesignRC provenance remain historical evidence, as explained in the
documentation guide.

Updated current documentation for:

- Weight and Balance, material densities, transient mass-statistics caching and
  toolbar order; Inspect regeneration, visibility and persistent export names.
- Wingspan-only manual Reference calibration and shared 3D camera controls.
- Profile circles, copy/paste/movement, orphan recovery and deletion, shared
  individual-curve selection/deletion, and project-wide Undo/Redo.
- Non-destructive flat fuselage end registration and the end-station tolerance.
- Current project format 28, compatible reads of versions 1–28, and transient
  history/generated geometry.

The proposed half-fuselage mirroring optimization is not implemented. No new
performance claim or benchmark is made by this audit.

Relative Markdown links were checked across the documentation tree. The two
original relative license/notice links in the archived DesignRC README remain
unchanged as provenance; current documentation links resolve.

## Validation

Wing/Fuselage generation suites are intentionally excluded under AGENTS.md.
Earlier full-regression reports record their dated revision and are not claimed
as a full regression run of this final revision. Latest focused checks and Debug
build/launch results are recorded below.

- Debug application and the selected test targets rebuilt successfully.
- Passed 12 focused CTest checks: editor_history_tests, weight_balance_tests,
  circle_profile_tests, profile_operations_tests, profile_recovery_tests,
  fuselage_end_tests, control_surface_editor_tests, station_sketch_tests,
  sketch_editor_tests, wing_workflow_tests, reference_tests, view_controls_tests.
  The first 11 completed in 16.96 seconds; camera controls completed in 2.66 seconds.
  These checks do not generate Wing/Fuselage components.
- Visually inspected the camera-control GUI capture, including the Reference
  scaling controls, primary toolbar order and bottom-centered camera buttons.
- Launched the rebuilt Debug application successfully.
- `git diff --check` passed. No generated build/test artifacts are included.
