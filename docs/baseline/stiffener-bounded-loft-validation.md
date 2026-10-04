# Bounded stiffener loft validation — 2026-10-04

## Change

Replace the global smooth cubic stiffener loft with ruled spans for strip and
round groove tools and carbon stock. Input settings, 65 samples, clearance and
bend checks, right-half cutting/mirroring, and OCCT parallel processing remain.
No small solids are discarded and no project format changes are required.
See ADR-0055 and the preceding diagnostic record.

## Regression fixture

`tests/fixtures/babybuzzard-stiffener-route.txt` contains the 65 numeric route
positions extracted during the original BabyBuzzard36 diagnosis, in millimetres.
It omits the project file and reference image. `stiffener_route_tests` uses a
matching ruled body to isolate groove construction from slower upstream stages.
For strip and round stock it checks a single valid resulting solid, stock volume,
and the finished cut at 25%, 50%, and 75% of every route span. The shallow groove
must be empty, deeper foam must remain, and foam on either side must remain.
The first targeted Debug run passed in 55.29 seconds.

## Commands

Builds use `--parallel 8`. Windows MSVC Debug and Release presets use the
repository's configured Qt and OCCT installations.

- `cmake --build --preset windows-debug --parallel 8`
- `ctest --preset windows-debug --output-on-failure`
- `cmake --build --preset windows-release --parallel 8`
- `build/debug/Debug/fuselage_benchmark.exe <original BabyBuzzard36.foam>`

The original project SHA-256 remains
`dba4db165b5a6d76ef042af04b8163eda7489ca38137d4dfab3052c1e5150ada`.
Its one mirrored Strip per side remains 3 × 1 mm, Start 20%, Stop 75%.

## Original BabyBuzzard36 regeneration

The unmodified saved project completed the full Debug generation successfully:
right-half hollowing, inserts and retaining rails, stiffener grooves, reflection,
holes, saved split, alignment pins/sockets, and display meshing. Final validation
reported nine valid project solids with total volume 703838.6184198704 mm³.
The run completed in 1526.17 seconds while builds and the full suite also ran;
this is validation evidence, not a controlled performance comparison.

Log: `build/debug/babybuzzard-fixed-generation.log` (ignored build output).
The original file remains unchanged.

## Full-suite and build results

The complete Debug CTest suite completed successfully: **64 active tests passed,
zero failures**, with the existing empty `designrc_geometry_tests` placeholder
intentionally skipped (65 registered entries). Total wall time was 1934.39 seconds.
This includes the new route regression, strip/round and curved stiffener
regeneration, full/mirrored symmetry, cancellation, support rails, fuselage cache,
Wing and Fuselage generation, persistence, GUI, mass properties and STEP export.

Both full Debug and Release builds succeeded with `--parallel 8`.
Logs are ignored build output:

- `build/debug/stiffener-fix-all-tests.log`
- `build/debug/stiffener-fix-full-build.log`
- `build/release/stiffener-fix-build.log`

Windows was validated; Linux/macOS were not exercised in this environment.
