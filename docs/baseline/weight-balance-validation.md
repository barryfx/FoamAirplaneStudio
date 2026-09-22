# Weight and Balance validation

Date: 2026-09-20. Platform: Windows, Debug configuration.

## Initial feature build and checks

- `cmake --preset windows-debug` completed successfully.
- `cmake --build --preset windows-debug --target designrc weight_balance_tests --parallel 4` completed successfully.
- `ctest --preset windows-debug -R '^weight_balance_tests$' --timeout 120` passed 1/1 tests (2.16 seconds; 2.21 seconds total).
- Only the dedicated Weight and Balance test was run. No Wing or Fuselage generation tests or broader generation suites were run.
- Launched the rebuilt `build/debug/Debug/foamairplanestudio.exe`; its Untitled window was running and responding.
- `git diff --check` passed.

The checks use analytical OCCT boxes for foam and plywood volumes and centroids,
including separate former/servo-tray inserts and prevention of fuselage double
counting. They verify weighted moments, density changes, root leading-edge datum,
Assembly placement/cut snapshots, and unavailable results when geometry is stale.

Qt editor checks cover Add/Edit/Cancel/Delete, unique names, grams/ounces and
Reference unit conversion, rectangle dimensions, selection and dragging, density
editing, dirty state, workspace visibility, save/reopen, old-format defaults,
invalid persisted inputs, and New project reset. These checks do not generate
aircraft components.

## Visual validation

Inspected the rendered synthetic-fixture screenshot at
`build/debug/weight-balance.png`. The final toolbar action follows Export;
instructions, part controls and both densities are visible. The side outline and
labeled component are framed on entry. The compact bottom status displays total
mass and signed center of mass in Reference units. Parts are hidden in other
workspaces. Screenshot and build logs are local build artifacts, not source files.

## Limits

Validation uses synthetic geometry, not a generated aircraft or physical weighing.
Linux and macOS were not exercised. Full model totals require current Assembly
geometry; reopening preserves inputs but does not generate geometry automatically.

## Cache and busy-status follow-up

The user subsequently authorized rebuilding and running all regression tests.
The full Debug build (`cmake --build --preset windows-debug --parallel 4`)
succeeded. The expanded Weight and Balance test passed in 2.38 seconds as part
of that full regression run.

The test observes the calculation status signal and verifies the WaitCursor is
already active, then checks that it is restored after calculation. It observes
one calculation on initial entry and no additional calculation after leaving and
returning unchanged. Changing and restoring Assembly placement each triggers a
calculation, as does replacing source solids with newly constructed equivalents.
Changing to cut geometry triggers another calculation; subsequent density edits
reuse the measured volumes. Save/reopen and stale geometry checks remain covered.
The post-calculation screenshot was inspected again and the bottom totals remain
visible and compact.

`ctest --preset windows-debug` completed in 1249.73 seconds: 38 passed, zero
failed, and one skipped out of 39 registered tests. `designrc_geometry_tests`
is an existing placeholder returning its configured skip code 77; it provides
no geometry coverage. The real Wing, Fuselage, stabilizer, Assembly, export,
persistence, and editor regression suites passed. Full output is retained locally
in `build/weight-balance-all-regressions.log`; build output is in
`build/weight-balance-regression-build.log`.
