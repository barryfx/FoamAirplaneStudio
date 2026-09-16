# Panel editors validation
Date: 2026-09-14
Environment: Windows Debug, Qt 6.11.1, OCCT 8.0.0.

Seven relevant tests passed in the initial implementation run:
- project_tests: 78.06 s; version-5 codec, backward compatibility and lifecycle.
- station_sketch_tests: 0.20 s; snapping/editing restricted to panel, identical
  boundary locations stored independently, per-panel completion and selection.
- airfoil_panel_tests: 0.89 s; independent assignments and tab choices, shared
  imported and traced entries, first-station selection and Escape retention.
- control_surface_editor_tests: 0.33 s; panel flags/hinges/rectangles, ownership,
  cross-panel overlap rejection and removal/re-add defaults.
- wing_solid_tests: 103.18 s; independent airfoil thickness near a shared panel
  boundary, separate panel flaps, eight mirrored solids and missing-panel error.
- spar_tests: 73.83 s; per-panel spars and intact moving control body, ten solids.
- station_workflow_tests: 74.25 s; full single-panel workflow and camera/data rules.

Final additional full-window validation covers tab restoration, independent
checkbox values, navigation without dirtying, save/open, missing-station toolbar
readiness and legacy global-control migration. Results and visual checks follow.

Documentation: ADR-0013, architecture and format version 5. Original project
files and running user sessions were preserved; DesignRC was not modified.
The workspace is not a Git repository, so no commit was made. Linux/macOS were
not tested. No performance claim is made.


The extra window test initially exposed floating-point drift when an idle
SketchEditor::finish recalculated station anchors during toolbar navigation.
Idle finish now leaves source geometry alone, and changing an outline tab emits
view/selection notification without resynchronizing unchanged anchors. The focused
panel workflow then passed (3.22 s), including save/open and clean navigation.
The three captured editor panels were visually inspected: numbered tabs, shared
library list, per-panel flap settings and station instructions display correctly.
Final regressions for this navigation correction follow below.


Final navigation regression run: all six passed (77.61 s total): project_tests
73.77 s, station_workflow_tests 77.55 s, airfoil_panel_tests 0.77 s,
sketch_editor_tests 0.68 s, designrc_gui_tests 0.35 s, station_sketch_tests 0.33 s.
The Debug build succeeded and the rebuilt app launched (PID 33652,
FoamAirplaneStudio). Existing app sessions remain available.
