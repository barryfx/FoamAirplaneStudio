# Former rotation validation

Date: 2026-09-19. Windows/MSVC Debug, branch Assembly. The user explicitly
requested the full test suite, including Wing and Fuselage generation, and an
application launch after validation.

Rotation Angle sits below Add Former, defaults to zero for each new former, and
edits the selection in decimal degrees. Negative angles rotate counter-clockwise
about the mask center in Side View. Rotated masks drive selection, local-axis
height editing, overlap checks, cavity fitting and retaining-rail placement.
Format 24 stores angles and reads all earlier projects with zero angles.

The full Debug build passed. Focused `former_tests` and `former_geometry_tests`
passed in 29.34 seconds combined. Added checks cover positive/negative tilted
volume and material probes, cavity clearance, rail contact/insert clearance,
rotated mask collision rejection, default/selection values, persistence and
legacy migration, invalid persisted angles, rotated dragging/resizing, and
regeneration when only the angle changes. Existing zero-angle checks still pass.

`build/debug/former-rotation.png` was visually inspected: the selected mask is
tilted counter-clockwise at -12.5 degrees, and Rotation Angle is directly below
Add Former. This is an editor screenshot; geometric direction and rail fit are
checked independently by the geometry assertions. Build logs and screenshots
are local artifacts, not committed.

The complete CTest run exposed an older station-workflow assertion that expected
Open to restore 3D and regenerate after editing an angle. The test now asserts
Open remains in 2D and leaves the model ungenerated until explicit 3D entry,
matching the existing requested behavior. No production change was needed for
that failure. `designrc_geometry_tests` is an intentional empty placeholder that
returns CTest skip code 77; actual geometry coverage comes from the component
suites, including the Wing/Fuselage/former checks above.

The workflow test also now sets its custom camera after the first regenerated
display's automatic fit, then verifies camera retention on subsequent rebuilds.
Its isolated rerun passed in 56.11 seconds. The complete run took 607.13 seconds;
after correcting and rerunning that test, all 35 implemented tests passed, with
the one intentional placeholder skipped. Logs:
`build/former-rotation-all-tests.log` and
`build/former-rotation-station-recheck.log`. The full build and updated test
target compiled successfully; `git diff --check` passed.
The rebuilt Debug application was launched and confirmed responding with its
FoamAirplaneStudio main window.
