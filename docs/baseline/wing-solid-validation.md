# Wing selection, tips and solid generation validation

Date: 2026-09-13. Windows MSVC Debug, Qt 6.11.1, OCCT 8.

- Normal `cmake --build --preset windows-debug --parallel 4` passed.
- Eight relevant CTest suites passed: wing_solid_tests, airfoil_panel_tests,
  station_workflow_tests, station_sketch_tests, sketch_editor_tests,
  wing_workflow_tests, reference_tests and designrc_gui_tests (41.19 seconds).
- After the final root-order and shading changes, wing_solid_tests and
  station_workflow_tests passed again (42.88 seconds). The only subsequent C++
  edit corrected a comment, followed by another successful normal Debug build.
- Geometry checks cover valid two-solid output, mirrored mass/bounds, full-span
  calibration, actual-scale coordinates, three tip shapes, multiple panels,
  profile interpolation, rounded spline tips, oblique stations independent of
  placement order, traced profiles and missing assignments.
- The volume symmetry test uses adaptive OCCT integration with 1e-8 relative
  tolerance. Default non-adaptive integration was insufficient for its original
  symmetry assertion; the test now measures the intended property accurately.
- Workflow checks cover automatic station selection, Escape/empty-click retention,
  locked station deletion, embedded tip icons and exclusive default selection,
  live/deferred regeneration, and clearing a model after prerequisite removal.
- Visually inspected `build/debug/wing-tip-smoke.png`: three vertically aligned
  image buttons, clear selected-tip border, mirrored shaded wing, and status.
  Construction seams are hidden in the single foam-body presentation.
- The rebuilt normal Debug app was launched. The previous running instance and
  its session were preserved by renaming its executable before linking. Runtime
  deployment now copies only changed OCCT DLLs so identical loaded DLLs remain
  untouched. Build log: `build/debug/wing-build.log` (generated, not source).

The legacy geometry-test shell was left unchanged. Linux/macOS were not run.
DesignRC was not modified. This folder is not a Git repository, so no commit was
created. No project-file persistence was introduced; ADR-0005 records the new
in-memory modeling conventions and approximation limits.
