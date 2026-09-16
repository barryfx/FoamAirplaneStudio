# Navigation, dirty data and camera validation

Windows Debug, 2026-09-13.

Six relevant tests passed (17.49 seconds): project_tests, station_workflow_tests,
airfoil_panel_tests, wing_workflow_tests, reference_tests and designrc_gui_tests.

The workflow regression visits every enabled Wing tool, Fuselage and Wing again,
changes viewport, zoom and station selection, and verifies that no processing
stage or model revision occurs and the project remains clean. Opening after that
navigation produces no save prompt. Assignments unlock Fuselage; removing a station
locks it; reopening a complete project restores eligibility.

Camera checks compare eye, center, up, scale, field of view and projection across
both live and deferred tip changes. Actual changes mark the document modified.
Project tests also verify view-only New without prompting and that immediate edits
after New remain modified after queued events. Existing save/cancel/failure and
complete state-restoration tests pass.

The OCCT viewport smoke image is build/debug/navigation-camera-smoke.png; it shows
the updated wing from the retained camera with Fuselage available. The final Debug
build succeeded. No geometry algorithm or persistent format changes were made.
Linux/macOS were not tested. No changes were made in DesignRC.
