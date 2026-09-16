# Airfoil station validation — 2026-09-13

Normal Windows Debug build passed. Focused CTest run passed 6/6 suites in 2.31
seconds: station_workflow_tests, station_sketch_tests, sketch_editor_tests,
wing_workflow_tests, reference_tests and designrc_gui_tests.

Interaction tests cover hover tolerance under zoom, consecutive placement,
horizontal/vertical snapping and the 15-degree boundary, line and fitted-spline
attachments, click/move/click endpoint movement, paired endpoint constraints,
unavailable intersections, Escape rollback, selection/deletion, inactive-mode
locking and dependent-station removal when a source curve is deleted.

The MainWindow integration test enters Reference dimensions, draws an outline,
selects Airfoil Stations, places two stations, checks Airfoils availability,
leaves the mode, and verifies station edits are locked. It then re-enters and
deletes a station, confirming that Airfoils is disabled again. Outline points
remain unchanged throughout station editing.

The captured MainWindow image was visually inspected for instruction layout,
toolbar state, distinct station lines and visible locked outlines. The rebuilt
build/debug/Debug/foamairplanestudio.exe was launched. Linux/macOS were not tested.

## Dropping a moved station

Left-click and Escape now both commit the current valid position and end movement.
Regression checks confirm that both endpoints retain alignment and subsequent
mouse motion no longer moves the station. Clicking beyond the paired curve's
available intersection drops at the last valid location. The focused station
sketch and MainWindow workflow suites passed (2/2, 1.24 seconds). Debug was rebuilt
and launched. Escape still cancels incomplete new-station placement.
