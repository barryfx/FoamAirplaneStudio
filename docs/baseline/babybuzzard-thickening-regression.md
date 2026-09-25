# BabyBuzzard thickening and retry regression

Date: 2026-09-23. Branch: `codex/fuselage-speedup`.

## Reproduction and cause

A read-only copy of the user's BabyBuzzard.foam reproduced:
“Wall thickness closes the cavity inside the fuselage. Reduce thickness near
the narrow section.” The project already uses format 28; no migration is needed.
The original project file was not edited.

The error occurred during planar section offsets, before cavity lofting or
reflection. OCCT's Intersection offset returned no result for multiple mirrored
sections around X=75–97 mm and X=163–177 mm despite widths around 63–67 mm and
walls of 6–9.4 mm. The section-continuity check interpreted those offset failures
as an interior cavity closure. The short sampled convex corner edges are consumed
by the inset, making a direct adjacent-line inset insufficient too.

## Correction

Retain the normal kernel offset. Only when it fails without a result, intersect
inward-shifted polygon lines, removing consumed convex edges. Reject consumed
concave edges, self-intersections, nonfinite/degenerate edges, source-boundary
crossings, and edge-to-boundary distances below the requested wall (1e-6 mm
numeric tolerance). Original inside and positive-area checks remain. Valid
kernel results with multiple/empty loops are never replaced. Profile sampling,
saved wall thicknesses and project format remain unchanged.

A failed Fuselage job now marks a transient retry state. Normal refreshes continue
to suppress repeated attempts, while explicit 2D-to-3D navigation clears the
attempt fingerprint. Worker startup and display failures use the same policy.
The state resets on New/Open and successful new attempts.

## Validation

`fuselage_wall_offset_tests` passes for the captured BabyBuzzard section, opposite
winding, symmetry, an ordinary rectangle, a truly consumed narrow section and a
two-chamber polygon whose connecting neck collapses. `view_controls_tests` passes
with an injected failed Fuselage job and actual 2D/3D tab changes; retry resets
without changing project data or running geometry in that GUI test.

Logs remain local under `build/debug/`: `babybuzzard-thickness-failure.log`,
`babybuzzard-thickness-retest.log`, `babybuzzard-focused-tests.log`, and
`babybuzzard-symmetry-tests.log`. These are correctness checks, not performance
benchmarks; builds and tests may run concurrently.

The seven-case `fuselage_symmetry_tests` suite passed (377.50 seconds while the
BabyBuzzard build was also running): full serial versus half parallel geometry
for solid and hollow models, open ends, variable walls, whole tray/rotated formers,
retainers, one-wall holes/hatches, oblique cuts and pointed ends.

The full copied BabyBuzzard project completed generation, display meshing and
validity checks successfully: two valid solids, volume 1273832.571966807 mm³.
The diagnostic run took 1501.22 seconds, mostly exact pin containment; it was
not an isolated timing benchmark. This full run preceded the final tightening
that prevents fallback on successful empty/multiple-loop kernel results; the
final build passed the offset and symmetry tests described above.
The final Debug application was rebuilt, launched and confirmed responding.
The benchmark target was relinked after its running validation process exited.

The original file still matches the frozen copy byte-for-byte, SHA256:
`480259e4f1a1bf620003fa8302653d2836bee14a2f3b27cbc13213455509c930`.
