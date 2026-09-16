# Mixed-unit entry and cursor zoom validation

Date: 2026-09-15. Windows Debug, Qt 6.11.1 / OCCT 8.

- Rebuilt Debug with `cmake --build --preset windows-debug --parallel 4`.
- Relevant CTest suites passed: reference_tests, spar_panel_tests,
  designrc_gui_tests, project_tests, sketch_editor_tests, station_sketch_tests,
  control_surface_editor_tests (7/7, 99.37 seconds).
- After removing unnecessary zoom margin padding, rebuilt the Debug app and
  viewport test; designrc_gui_tests passed again (0.65 seconds). Its tests cover
  cursor position after explicit scrolling, scrollbar transitions, wheel and
  pixel deltas, zoom-out margins, Fit View and restored zoom/center.
- Reference tests cover mm in inch projects, inch values in millimetre projects,
  bare values, project-unit changes, invalid input and readiness.
- Spar tests cover independent dimension text, canonical millimetre values,
  unit changes, invalid edits, panel navigation and restoration.
- Project tests cover version migration, mixed-unit text round trips, New/Open/
  Save/Save As/Close, unsaved changes, embedded images, drafts and camera state.
- Visually inspected `build/debug/mixed-unit-spars.png`; entered numeric text
  and suffixes are readable and retained in the panel.
- Checked for workspace FoamAirplaneStudio processes before each build and
  applied the requested stop-before-build policy. No app process was running.
- Launched the rebuilt Debug app after validation. No DesignRC files changed.

Existing OCCT deprecation and Qt deployment VCINSTALLDIR warnings remain.
Linux/macOS have not been exercised in this environment. No performance claims
or geometry-algorithm changes are made by this update.
