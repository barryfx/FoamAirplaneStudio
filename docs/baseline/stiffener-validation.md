# Fuselage stiffener validation — 2026-10-03

Windows x64, MSVC 18.5, Qt 6.11.1, repository OCCT Debug/Release runtimes.
The user explicitly authorized full generation tests for this change.

`cmake --build --preset windows-debug --parallel 4` and
`cmake --build --preset windows-release --target designrc --parallel 4` succeeded.
Existing OCCT deprecation and Qt deployment VCINSTALLDIR warnings remain.

New `stiffener_tests` checks:

- One centered and three equally spaced grooves per side on analytical solids.
- Mirrored strip removal and full stock volume; round half-depth groove volume.
- Curved cross-section fitting and invalid wall depth/range rejection.
- Carbon mass and centroid preservation during wing Assembly placement.
- Full hollow-fuselage generation with two strips per side and one round rod
  per side, retaining valid topology and expected stock properties.
- A spline-defined curved hollow boom with smooth, non-collinear round grooves.
- Project format 33 round trip, version 32 disabled defaults and malformed data.
- Project-unit conversion, explicit units, invalid entry preservation and
  Strip/Round field visibility.

`editor_history_tests` additionally checks stiffener undo/redo and independent
wing/fuselage fingerprints. `fuselage_thicken_tests` additionally exercises the
real Stiffeners toolbar action, count editing and save/reopen.

The native-window `build/debug/stiffeners.png` capture was inspected: all fields
are readable and three evenly spaced mirrored grooves appear in the 3D fixture.
Generated build/capture artifacts are ignored and are not committed.

Full Debug command: `ctest --preset windows-debug --output-on-failure --timeout 1200`.
The 61-entry suite finished in 1517.95 seconds. Two stale tests initially failed:
the stabilizer toolbar omitted the existing Fiberglass tab; the holes checks
expected format 31 and assumed Holes was the last tab. Those assertions were
corrected and the rebuilt `stabilizer_outline_tests` and `former_tests --holes`
both passed complete reruns. The extended curved-boom `stiffener_tests` also
passed after its final rebuild. Final aggregate: 60 active tests passed.
`designrc_geometry_tests` intentionally skips with code 77 because its source is
an empty placeholder; actual Wing/Fuselage generation suites ran and passed.

The rebuilt Release executable was launched from
`build/release/Release/foamairplanestudio.exe` (PID 35184). Its window title was
FoamAirplaneStudio, the process remained alive and Windows reported it responding.
Debug and Release help deployment was refreshed after the final format-reference
correction. Linux/macOS builds were not exercised.

## Follow-up: defaults, units and cutting before reflection

Default Start/Stop is now 20%/90%; count is restricted to 0–3 per side. GUI tests
verify bare mm entry in an inch project, independent explicit inch display,
invalid entry preservation and reset defaults. Earlier saved counts above three
load capped at three; saved percentages are retained.

Grooves now cut the right half before reflection. Tests verify the reported stage
order, right-half removal volume, mirrored carbon centroids and volumes, and full
regeneration of straight and curved hollow booms. Full serial versus mirrored
parallel parity includes both Strip and Round, comparing geometry and carbon
properties. Cancellation checks include the stiffener cutting stage.

Only test executables were rebuilt; neither Debug nor Release application was
built or launched for this follow-up. The user authorized fuselage regeneration
after initially requesting GUI-only validation. Six selected Debug tests passed
in 233.81 seconds: stiffener_panel_tests, editor_history_tests, stiffener_tests,
fuselage_symmetry_tests, fuselage_cancellation_tests and fuselage_thicken_tests.
The final panel test was rebuilt/rerun after improving the count-validation error
message and adding its regression assertion. OCCT parallel Boolean processing
remains on by default. No performance improvement is claimed or benchmarked.
