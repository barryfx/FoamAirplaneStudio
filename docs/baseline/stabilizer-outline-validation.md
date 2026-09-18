# Stabilizer outline validation
Date: 2026-09-17

Windows MSVC Debug, Qt 6.11.1 / OCCT 8.

The full Debug build succeeded. After the final editor activation adjustment,
`cmake --build --preset windows-debug --target designrc stabilizer_outline_tests --parallel 4`
also succeeded. Existing Qt deployment environment warnings remain nonfatal.

Final `stabilizer_outline_tests`: passed, 2.80 seconds (2.83 seconds total CTest).
No Wing/Fuselage generation is exercised by this suite. Coverage includes:

- Both toolbars, independent editing, Line/Spline snapping, dragging, selection
  and deletion, 2D/3D locking, and component-specific instructions.
- Endpoint-height warnings, relative tolerance and translation/scale invariance,
  mixed/reversed curve ordering, branches, closed and disconnected paths.
- Save/Open of both outlines and unfinished splines, active tools, clean dirty
  state after restoration, New/Close, reference remapping, version-15 migration,
  removed tool-name migration, invalid-file rejection and preservation of current data.

Both component screenshots were visually inspected: instructions and buttons fit
the data panel, the four requested toolbar actions appear, and the open curved
outline and editable nodes render. Captures are generated build output at
`build/debug/stabilizer-outline-0.png` and `stabilizer-outline-1.png`.

Before the user's instruction to stop Wing/Fuselage generation testing, nine
existing suites passed: fuselage_outline_tests, fuselage_profile_tests,
former_tests, servo_tray_tests, fuselage_cut_tests, fuselage_thicken_tests,
sketch_editor_tests, wing_workflow_tests and reference_tests.
project_tests initially failed because its unsupported-version fixture hard-coded
16, now a supported version. The fixture now uses current version plus one.
Its rerun was stopped at the user's instruction because it includes component
generation; CTest records it as failed/interrupted, not a completed pass.
No further Wing/Fuselage generation tests were run. AGENTS.md records this restriction.

The workspace application was stopped before rebuilding, and the rebuilt normal
Debug application was launched successfully. Linux/macOS were not tested.
Airfoil selection, Hinge Line, Cut and stabilizer solid generation remain future work.

## Endpoint angle revision

The endpoint line now passes within 10 degrees inclusive of horizontal or vertical,
replacing the height comparison. Tests cover all four axis directions, both sides
of each axis at 9.999, 10 and 10.001 degrees, diagonal rejection, reversed traversal,
translation, uniform scaling, interior-curve independence and coincident endpoints.
Both component UI tests also leave a 90-degree rotated outline without a warning
and verify the revised instructions and warning text.

Debug app and stabilizer test targets rebuilt successfully. The focused stabilizer
suite passed in 2.56 seconds (2.61 seconds total CTest). No Wing or Fuselage
generation tests were run for this revision. The prior workspace app was stopped
before building and the rebuilt Debug app was launched.
