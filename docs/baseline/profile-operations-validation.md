# Profile copy, paste and move validation

Selection-color follow-up: selected profiles now use orange with a white border,
including copy picks and whole-profile dragging. Rendered-color checks verify
station selection and clearing/restoring the highlight across workspace changes.
The focused profile operations, Circle and sketch editor tests passed in 3.87
seconds. Inspected `build/debug/profile-highlight.png`; the selected profile is
orange and the other profiles remain blue. Debug application rebuilt successfully.
No generation suites ran for this visual-only follow-up. Logs:
`build/profile-highlight-build.log` and `build/profile-highlight-tests.log`.

Date: 2026-09-21. Windows Debug.

Added Copy Profile source picking, independent Paste Profile assignment and
whole-profile press/drag/release movement. Paste finds space to the right of
existing drawing bounds and centers the viewport. Reopening and dropping moved
profiles extend the canvas to keep profiles reachable through Fit View/scrolling.
Existing profile persistence is reused without a format change.

The full Debug build succeeded. Initial focused profile/circle tests passed in
2.80 seconds. Visual inspection exposed the missing restored canvas bounds; this
was fixed and the application/profile test executable rebuilt before the full
run reached the profile suites.

`ctest --preset windows-debug` completed in 940.02 seconds: 42 passed, zero failed,
one existing `designrc_geometry_tests` placeholder skipped (code 77). The user
explicitly authorized this full run, including Wing/Fuselage generation tests.
Project tests were rebuilt for the canvas follow-up. Final project lifecycle,
Circle and profile operations checks all passed in 58.93 seconds.

Checks cover tool exclusivity, copying without dirtying a saved project, source
snapshot independence, destination assignment, replacing an existing profile,
shared-slot independence, clear paste placement, viewport centering, circle and
multi-curve Line/Spline profiles, rigid translation, release stopping movement,
station assignment preservation, Save/Open, and restored canvas bounds.

Inspected `build/debug/profile-operations.png` after the fix: Copy/Paste and Move
buttons are readable, and Fit View includes all copied profiles after reopening.
Local logs: `build/profile-operations-build.log`,
`build/profile-operations-bounds-build.log`,
`build/profile-operations-project-build.log`,
`build/profile-operations-regressions.log`, and
`build/profile-operations-final-checks.log`. Linux/macOS were not exercised.
