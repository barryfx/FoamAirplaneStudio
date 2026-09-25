# Startup splash and About validation

Date: 2026-09-25

Built Debug `designrc` and `view_controls_tests`. Ran only
`ctest --test-dir build/debug -C Debug -R '^startup_gui_tests$' --output-on-failure`.
Passed; no model-generation tests or geometry builders ran.

The GUI check verifies the embedded splash image, screen-bounded square sizing,
clicks not dismissing it early, a three-second startup interval, event-loop
responsiveness (140 timer callbacks), transition to the maximized MainWindow,
and no active model jobs. It opens the real Help -> About action and checks the
workflow description, Codex credit, retained copyright/license/library credits,
and removal of development/DesignRC-shell wording and the local license path.
Help has exactly one action: About. Both captured images were visually inspected.

Evidence: `build/debug/startup-splash.png`, `build/debug/startup-about.png`,
`build/debug/startup-gui-tests.log`, `build/debug/startup-final-build.log`.
Debug app launched after successful checks. Whitespace validation passed.
