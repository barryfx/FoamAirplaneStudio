# Project persistence validation

Date: 2026-09-13. Windows MSVC Debug, Qt 6.11.1, OCCT 8.

Normal Debug build passed. Nine relevant CTest suites passed (33.99 seconds):
project_tests, airfoil_panel_tests, station_workflow_tests, station_sketch_tests,
file_dialog_tests, sketch_editor_tests, wing_workflow_tests, reference_tests and
designrc_gui_tests. Build log: build/debug/project-final-build.log.
After the final New-project reset correction, the normal Debug build passed
again and project_tests/designrc_gui_tests passed (2/2, 17.01 seconds), including
a check that New clears a saved hidden-view position and starts clean.

ProjectTests covers portable embedded multi-page references with missing source
paths, physical/manual scale and units, imported/traced airfoils, station
attachments/assignments, named unfinished spline drafts, selected modes/panels,
actual 2D zoom/center restoration, 3D regeneration/camera restoration and deferred
restoration of the hidden viewport. It exercises the actual Open and Save As
dialogs, saves on Close, canceled New/exit, canceled Save As during Close, failed
writes, malformed/unsupported files and cross-reference rejection. Initial and
restored projects have clean modification state.

Visually inspected build/debug/project-restored-smoke.png: the saved Wing Tip
selection and 3D camera were restored, retaining the intentional close-up instead
of automatically fitting away from it. Embedded image pixel content was compared
in tests. File dialogs use temporary settings and non-native Qt dialogs so user
history is untouched.

The legacy sketch viewport test had a brittle exact single-pixel assertion at
fractional display scaling. The captured outline was visibly correct. Its check
now requires the exact light-blue outline color within two physical pixels of
the rounded sample location; direct scene-space color assertions remain intact.
Failures in that test now report to stderr instead of blocking on a Windows
assertion dialog.

The rebuilt normal Debug application was launched. No DesignRC files were
modified. Linux/macOS were not run. The working directory has no Git repository,
so no commit was created. ADR-0006 and foam-project.md document the format.
