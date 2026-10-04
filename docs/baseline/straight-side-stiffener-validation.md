# Straight side-view stiffener validation — 2026-10-04

## Behavior

The user confirmed that straight means straight in Side View while following the
skin laterally to keep constant inward depth. Endpoint height fractions now define
X/Z; the local midpoint at intermediate sections no longer changes the path.
The count, mirrored-side behavior and saved stock dimensions are unchanged.
See ADR-0056 for lateral sampling tolerance and the distinction from a straight
3D axis. The panel description states the behavior explicitly.

## Checks

`stiffener_route_tests` covers one and three Strip/Round stiffeners on the captured
BabyBuzzard numeric route. Finished cuts are checked between every pair of route
samples, and carbon volume/centroid checks verify the new endpoint placement.
A changing trapezoidal side profile is tested against analytic skin intersections,
with probes 0.015 mm on either side of the nominal 2 mm depth. A separate case
rejects a straight side-view line that leaves the outline between valid endpoints,
reports a percentage, and leaves previously collected stock unchanged.

The actual BabyBuzzard pre-stiffener body and wall sections saved during the
previous investigation were reused for an isolated cut check. The unchanged
one-per-side 3 × 1 mm Strip, Start 20%, Stop 75%, produced one valid right-half
solid and one right-side carbon-stock record with the revised path. This check
reuses the already generated hollow body/support rails; it does not rerun the
saved project's later holes, split, pins or meshing. General full-generation and
mirroring/cancellation cases are included in the relevant suite.

Diagnostic log (ignored): `build/debug/babybuzzard-straight-skin-check.log`.
Temporary diagnostic source/targets were kept out of the repository changes.
The user's project file was not edited.

Builds use `--parallel 8`; Windows Debug tests and a Windows Release build were
used. Linux and macOS were not exercised.

## Results

Six relevant Debug CTest entries passed, zero failures, in 625.13 seconds:
`stiffener_route_tests`, `stiffener_tests`, `fuselage_cache_tests`,
`fuselage_symmetry_tests`, `fuselage_cancellation_tests`, and
`stiffener_panel_tests`. After adding the analytic changing-profile and rejection
checks, the final rebuilt `stiffener_route_tests` passed its complete rerun in
255.26 seconds. Logs: `build/debug/straight-stiffener-tests.log` and
`build/debug/straight-route-final-tests.log`.

Release compilation and deployment succeeded with `--parallel 8`.
Log: `build/release/straight-stiffener-build.log`.
