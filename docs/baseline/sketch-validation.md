# Wing sketch validation — 2026-09-13

Windows Debug configure/build passed using the normal windows-debug preset.
Focused CTest run: sketch_editor_tests, wing_workflow_tests, reference_tests and
designrc_gui_tests passed (4/4, 1.44 seconds).

The sketch regression exercises real Qt mouse/key events, button clicks, tab and
spinner changes, and rendering of line/spline layers. It covers shared-point
snapping and dragging, panel isolation, Escape behavior, connectivity readiness,
reference reload preservation and physical scale round trips.

The rebuilt build/debug/Debug/foamairplanestudio.exe was launched successfully.
Visual inspection confirmed the Wing/Outline spinner, numbered tab, wrapped
instruction, side-by-side Line/Spline buttons and loaded reference background.
The active user session was observed without injecting mouse or keyboard input.
Linux and macOS were not tested. No DesignRC files were modified.

## Curve selection and deletion

The subsequent selection/deletion change passed the same four focused suites
(4/4, 1.54 seconds). Added checks cover line and fitted-spline hit testing,
zoom-independent tolerance, selection clearing, Delete guards, orphan-point
cleanup with shared endpoints retained, panel isolation and inactive rendering.
The captured Qt viewport image was visually inspected: the selected curve is
thick orange while the connected unselected curve remains blue. The normal
Debug application was rebuilt and launched successfully.

## Outline contrast outside editing

Completed outlines now use light blue (80, 200, 255), three screen pixels wide,
with a five-pixel dark blue backing stroke in all modes. The inactive purple
style was removed. Tests check contrast on black and white backgrounds and
actual viewport rendering with the editor inactive and its panel hidden.
The viewport capture was visually inspected. Scroll position is explicitly set
in the regression so the tested outline lies inside the visible area.
Windows Debug build passed; sketch_editor_tests, wing_workflow_tests and
designrc_gui_tests passed (3/3, 1.09 seconds). The normal Debug app was launched.

## Multi-panel outline completion

Panel validation now explicitly checks each connected chain's open endpoints,
and whole-wing readiness checks every panel. A three-panel interaction regression
draws segmented LE/TE lines on panel 1, separate fitted splines on panel 2, and
LE/TE plus a tip on panel 3. It verifies readiness across tab changes, rejection
of an inner tip connection, deletion/recreation of an inner edge, and panel-count
changes. Windows Debug build passed; sketch_editor_tests and wing_workflow_tests
passed (2/2, 0.90 seconds). The rebuilt normal Debug app was launched.
