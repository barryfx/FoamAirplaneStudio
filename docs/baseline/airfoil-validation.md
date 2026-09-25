# Airfoil library validation — 2026-09-13

## Trailing-edge cap regression — 2026-09-23
BabyBuzzardAifoil.dat repeated its upper TE at the end, after a lower point at
X=0.9978670881, Y=-0.0074131559. Treating this closing edge as lower surface data
pulled the fit upward. The fitter now identifies this steep terminal cap and
extends the adjacent surface tangent to X=1 before fitting. A fixture copied from
the user's file covers strengths 0/25/100, no terminal hook, positive thickness,
reversed traversal, exact vertical closure and unchanged source data. Existing
sharp/blunt endpoint tests still pass. `airfoil_panel_tests` passed in 1.73 seconds;
Debug built successfully. The 1000x680 preview was captured and visually checked.
Evidence: `build/debug/airfoil-trailing-edge-build.log`,
`airfoil-trailing-edge-tests.log`, `babybuzzard-trailing-edge.png`.

## Smoothing preview — 2026-09-23
Debug application, airfoil panel and persistence UI targets built successfully.
`airfoil_panel_tests` and `project_persistence_ui_tests` passed. The numerical
fixture checks reduced error against a known smooth contour with added trace
noise at four strengths, positive interior thickness, fixed LE and sharp/blunt TE
points, nearly vertical sampled nose segments, symmetric results, strength effects,
rejection of crossed/zero-thickness input and immutable source data. GUI coverage
exercises preview/cancel, slider, name validation, Save Copy, unchanged assignments,
library restore and 69-point export. The persistence UI test saves/reloads the
321-point copy and exercises real project Undo/Redo without generating models.
The preview screenshot was visually inspected for the two outlines, readable
metrics, slider and Save Copy controls. No Wing or Fuselage generation tests ran.
Evidence: `build/debug/airfoil-smoothing-build.log`, `airfoil-smoothing-tests.log`,
`airfoil-smoothing-persistence-tests.log` and `airfoil-smoothing.png`.

## 69-point export limit — 2026-09-23
Reduced export to 35 cosine samples per surface sharing one LE, for 69 coordinate
rows excluding the name header. Updated checks passed in `airfoil_panel_tests`
(1.39 seconds), covering the point count, spacing, endpoints and round trips for
imported and traced profiles. Debug rebuilt successfully. Evidence:
`build/debug/airfoil-69-build.log` and `build/debug/airfoil-69-tests.log`.
The 201-point validation below records the earlier export implementation.

## DAT export validation — 2026-09-23
Debug application and airfoil/Assembly test targets built successfully.
`airfoil_panel_tests`, `assembly_tests` and `export_tests` passed (20.24 seconds
total). No Wing or Fuselage generation tests were run. Airfoil coverage includes
201 cosine-spaced samples, exact X endpoints, shifted/scaled imports, Lednicer
input, sharp and blunt trailing edges, periodic and blunt traced profiles,
round-trip import, invalid selection and write failure, the actual selected-entry
Save dialog, cancellation, disabled export during tracing and reset. The captured
panel was visually checked for the export button and readable layout.
Evidence: `build/debug/airfoil-former-tests.log`, `airfoil-export-build.log`,
and `airfoil-export.png` (local build artifacts).

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
