# Fuselage outline validation

Date: 2026-09-16. Windows Debug, MSVC, Qt 6.11.1 / OCCT 8.

## Build and tests

Full Debug build passed using `cmake --build --preset windows-debug --parallel 4`.
The final focused CTest run passed all three suites in 2.85 s:

- fuselage_outline_tests: 1.92 s. Real button/mouse/key interaction covers Top/Side
  mutual locking, Line/Spline drawing, periodic spline closure, point dragging,
  selection/Delete, readiness and action highlighting, and retained Wing inputs.
  Exit warnings identify Side alone, Top alone and both views (including 3D entry).
  Tests cover independent layers, branch/multiple-loop rejection, version-8
  defaults, version-9 round trips, invalid-file preservation, pending-draft
  Save/Open, clean navigation, New/Close, and reference-scale remapping.
- reference_tests: 0.22 s.
- designrc_gui_tests: 0.65 s (shared viewport behavior).

An earlier broader project/Wing-workflow run was interrupted at the user's request
not to run Wing-specific tests. It has no completed result and is not claimed as
passing. Wing-specific suites were not rerun. The dedicated Fuselage suite covers
its persistence paths without generating a Wing solid. The initial new test build
required a correction to Qt typed widget lookup; the final build succeeds.

## Visual and launch checks

Visually inspected build/debug/fuselage-outlines.png from the test window: Outline
is checked, Profile Stations is enabled, Side View is lit, Top View is disabled,
Line/Spline appear beneath Side View, and both the closed rectangle and closed
spline render with editing handles only on the selected view. Instruction text is
readable at the normal data-panel width. The small Wing fixture remains unchanged.

The rebuilt Debug app was launched and its window was confirmed running. Workspace
application processes were stopped before building; unrelated processes were not
changed. The existing VCINSTALLDIR deployment warning is nonfatal. Linux/macOS
were not tested. No fuselage solid generation or Profile Stations editor is claimed.

Documentation: ADR-0019, architecture/fuselage-outlines.md and format version 9.
Older project files are readable and are only upgraded on explicit Save.
# Open-endpoint markers, 2026-09-21

Follow-up: validation warnings now distinguish empty views, separate closed
loops, open endpoints, branching junctions and other invalid boundaries. The
Baby Buzzard project reproduced two closed loops in Top View and an empty Side
View, explaining the absence of red endpoints. With user approval, the lower
loop was moved to Side View after making a byte-for-byte backup; point positions
were preserved. The focused outline test checked the original file's specific
diagnosis and both repaired boundaries, and opened the repaired project in the
real MainWindow without generating models. It passed in 5.12 seconds total.
Debug application and test builds succeeded. Logs are in
`build/outline-diagnostics-build.log` and `build/outline-diagnostics-tests.log`.

Built the Debug application, `fuselage_outline_tests`, and `sketch_editor_tests`.
Both focused tests passed (4.30 seconds total). Added rendered-color checks for
unconnected ends, ordinary connected corners, closure, pending spline endpoints,
markers with neither view selected, and marker removal outside Outline mode.
Visually inspected `build/debug/fuselage-open-endpoints.png`: the open chain has
two red endpoints with white borders, and its connected corners remain blue.
No component generation suites were run. Build/test logs are local artifacts at
`build/outline-endpoints-build.log` and `build/outline-endpoints-tests.log`.
