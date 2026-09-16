# Control-surface validation

Windows Debug, 2026-09-14.

Eight relevant tests passed (42.76 seconds): project_tests,
control_surface_editor_tests, control_surface_tests, airfoil_panel_tests,
station_workflow_tests, station_sketch_tests, sketch_editor_tests and
designrc_gui_tests.

The extended project/editor/workflow checks subsequently passed (3/3, 42.21
seconds). They include real version-1 Open followed by version-2 Save As, controls
and pending-corner restoration, New/Close resets, and both control types in a
single mirrored 3D wing while retaining the camera. Malformed hinge values are
rejected.

Geometry tests verify two independent controls, exact tape top contact and
standard center contact, 45-degree gaps, removed volumes, invalid/overlapping
rectangles and four separate mirrored solids for a NACA wing with one control.
The combined viewport smoke fixture uses a slightly oblique root, a tape aileron
and a standard flap.

Visual smoke captures (generated build outputs):
- build/debug/control-rectangles-smoke.png: both conditional panels, distinct
  rectangle colors over the locked outline, and exclusive hinge choices.
- build/debug/control-wing-smoke.png: mirrored fixed/control bodies with visible
  standard-hinge relief and retained camera.

Debug build succeeded. Existing OCCT deprecation and Qt deployment environment
warnings remain. Linux/macOS were not tested. The previously running application
was preserved by renaming its executable before rebuilding; its process/data were
not stopped. No changes were made in DesignRC.

The final mode-status regression also passed (station_workflow_tests, 33.23 seconds).
