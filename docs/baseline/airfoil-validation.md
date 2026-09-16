# Airfoil library validation — 2026-09-13

The normal Windows Debug build passed. Nine relevant CTest suites passed in
10.03 seconds: airfoil_panel_tests, station_workflow_tests, station_sketch_tests,
file_dialog_tests, sketch_editor_tests, wing_workflow_tests, reference_tests,
designrc_domain_tests and designrc_gui_tests.

New coverage exercises the actual Load button/shared dialog, header names and
filename fallbacks, Selig/Lednicer input, rejected malformed files, and normalized
coordinates with a nonzero leading-edge X. It also checks exclusive radio choices,
independent station assignments, restoring choices on station selection, locked
station geometry, named line and closed periodic-spline traces, disabled loading
and assignment controls during tracing, open/multiple-loop warnings, name-dialog
cancellation and reset. Invalid-loop warnings were dismissed by the test to
verify that sketching can exit without adding an invalid library entry.

MainWindow integration checks that Airfoils exposes the requested panel and that
Wing Tip is enabled only after all stations are assigned. Captured panel and
MainWindow images were visually inspected, including the airfoil list and orange
selected-station highlight. The rebuilt build/debug/Debug/foamairplanestudio.exe
was launched. Linux/macOS and native file-dialog interaction were not tested.
Automated file-dialog tests use isolated temporary settings and non-native Qt
dialogs. No files in DesignRC were modified.
