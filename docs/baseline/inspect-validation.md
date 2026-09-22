# Inspect validation

Date: 2026-09-21. Windows Debug.

Built `designrc`, `inspect_tests`, and `view_controls_tests` using the
`windows-debug` preset. Ran
`ctest --preset windows-debug -R '^(inspect_tests|view_controls_tests)$' --timeout 300`.
Both tests passed: View controls in 2.39 seconds and Inspect in 10.56 seconds,
13.08 seconds total.

Inspect checks exercise the real MainWindow and OCCT viewport: component
checkboxes change displayed objects, name validation rejects invalid and duplicate
names, camera controls remain available, and 2D is disabled. Save/open preserves
format-27 aliases; format-26 projects load without aliases. Renamed components
appear in the Export checklist and actual STEP, STL and DXF output; the combined
STEP uses the project filename and is read back successfully.

A small defined Wing verifies Inspect regeneration after input changes, reuse of
unchanged geometry, cancellation without publishing partial results, and explicit
retry with names retained. No broad Wing/Fuselage generation or geometry
regression suites were run.

Visually inspected native-window captures `build/debug/inspect.png` and
`build/debug/inspect.png-hidden.png`. The synthetic component fixture confirms
the checkbox/name list, removal of the unchecked fuselage, disabled 2D tab and
bottom-centered camera controls. Build logs, test logs and captures remain local
artifacts. Linux/macOS were not exercised. Component naming across topology or
solid-order changes has the ordinal-identity limitation documented in ADR-0041.
