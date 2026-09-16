# Draw-mode indication and underside lighting validation

Windows Debug, 2026-09-14.

Four tests passed (41.16 seconds): control_surface_editor_tests, project_tests,
station_workflow_tests and designrc_gui_tests. Editor regressions check the lit
button on activation, exclusive aileron/flap targeting, completion, Escape,
click-to-cancel, restored drawing state and mode exit.

The OCCT bottom-view smoke capture (build/debug/underside-lighting-smoke.png)
shows the wing underside and hinge relief under the new symmetric lower fill
lights and increased ambient light. Camera retention and project restoration
regressions also passed. Debug build succeeded; no geometry or file-format change.
The prior application instance was preserved. Linux/macOS were not tested.
