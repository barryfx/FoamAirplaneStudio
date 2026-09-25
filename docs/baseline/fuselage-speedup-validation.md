# Half-fuselage generation and threading validation

Date: 2026-09-22. Local branch: `codex/fuselage-speedup`.
Baseline source: `1d8faf5b18f5d540b928ee86bf5841de48113035` on main.
The user explicitly authorized Fuselage generation benchmarking and additional
parallel processing. No Wing generation was run.

## Input and method

The supplied path contained an extra directory separator. The actual project is
`reference_imgaes/GentleLady.foam`, frozen as
`build/debug/fuselage-speedup-fixture.foam` before timing. SHA256:
`CFED9F8ED619737DBDAB1445391A3CEF92FD4874A3436690F228C1EC8786AFCF`.
The original file was not modified. It includes variable walls, a servo tray,
three formers, retaining rails, a Top wall hole, a Side cut and mating hardware.

Windows/MSVC Debug with OCCT Debug libraries, Intel Core Ultra 9 185H,
22 logical processors. Timers include complete Fuselage generation and display
meshing. Project reading, result serialization and parity checks are outside the
timer. No other build or test ran during the reported generation timings.

The old binary was retained as `build/debug/Debug/fuselage_speedup_baseline.exe`.
The benchmark now uses the same Wingspan calibration as the app for manually
scaled projects. This file is already to scale, so that harness correction does
not change its baseline geometry.

The user explicitly chose the right profile half as authoritative. The symmetric
model retains full Top View width around Y=0 and Side View height/vertical offsets.
Consequently, the original asymmetrically traced model is intentionally different.
The additional full-symmetric comparison uses the same new geometry and threading,
with `FOAM_BENCH_FULL_SYMMETRIC=1`, to verify the half-construction optimization
without treating intentional geometry changes as parity failures.

## Baseline

The original full-body build completed in 1151.52 seconds (19 minutes 11.52 seconds)
and produced seven valid solids. Volume: 844492.6094374442 mm³. Bounds in mm:
`[-0.0000001000000000416334, -25.9761802201526, -21.88524091141657,
1005.998502406573, 25.57293713760177, 38.00094456796538]`.

## Implementation

See ADR-0045. Main fuselage halves are never joined. Whole cavity tooling fits
whole formers/tray. Connected cut-piece groups use seam-face area and combined
volume to select the main body, including oblique Top cuts; only detached groups
are joined into whole removable parts. Holes/cuts precede pin placement.

Independent former fits, per-former rail construction, four pin searches, and
pin fusion/socket subtraction use bounded workers with private CAD copies.
Worker kernel operations retain the existing per-operation parallel policy and
use OCCT's shared native thread pool. Exact pin containment sums separate half
intersections, and former clearance subtracts body solids individually, avoiding
Boolean interference between touching halves. Conflicting pin candidates retry in
original order. No global kernel setting or persistent format changes.

## Focused validation

The new parity cases compare serial full-symmetric construction with threaded
half construction: solid profiles, closed cavities, open ends with varying walls,
whole trays and multiple rotated formers, retaining rails, a whole hatch with a
one-sided hole, oblique Top cuts, and pointed ends. They check validity, positive
volume, part counts, bounds, centroids, sampled material and cancellation.

The existing Thicken test needed corrections to its synthetic canvas bounds,
acceptance of the orange selected-profile highlight, and per-solid containment
checks for a multi-part result. A classifier loaded with the entire compound is
not a reliable union-of-solids membership test. Product UI code was unchanged.

An interrupted development benchmark is excluded from timing evidence. Its log
is `build/debug/fuselage-speedup-optimized-interrupted.log`.

## Measured results

| Build | Elapsed seconds | Elapsed minutes |
| --- | ---: | ---: |
| Original main baseline | 1151.520 | 19:11.52 |
| Intermediate mirrored implementation | 760.634 | 12:40.63 |
| Final mirrored implementation | 600.669 | 10:00.67 |
| Full-symmetric comparison, new threading | 1242.570 | 20:42.57 |

The final build takes 47.84% less time than the original baseline (1.92x speedup)
on this fixture. These are single Windows Debug runs, not a statistical estimate
or a prediction for Release builds or other models. The intentional symmetry
change is included in this comparison.

Relative to the intermediate mirrored implementation, the four pin searches
fell from 334.589 to 215.420 seconds, and pin fusion/socket subtraction fell
from 53.723 to 23.220 seconds. These timings include the combined effects of
per-solid containment and shared native kernel parallelism; they do not isolate
the benefit of task concurrency alone.

The final result contains seven valid solids and has volume
847163.8775117816 mm³, about 0.316% above the original asymmetric baseline.
Bounds in mm: `[-1e-7, -25.74970738359015, -21.88524091141657,
1005.998502406573, 25.74970738359015, 38.00094456796538]`.
Against the intermediate mirrored BREP, solid count, volume (one part per million),
bounds (1e-5 mm), and 429 three-axis ray hit counts/positions (1e-5 mm) passed.
Both have 7691 faces.

Logs and BREP artifacts are local under `build/debug/`:
`fuselage-speedup-baseline`, `fuselage-speedup-before-partition`, and
`fuselage-speedup-optimized` (each with `.log` and `.brep` extensions).
Generated artifacts are not committed.

The full-symmetric comparison took 1242.57 seconds and produced volume
847163.8775864633 mm³ with the same bounds. Its pin searches took 721.939 seconds,
versus 215.420 seconds for the optimized per-half containment path. Its pin/socket
operations took 27.360 seconds. This comparison includes the different full-body
containment and construction paths; it is not an isolated loft-only benchmark.
The full-symmetric model has 7687 faces versus 7691 for the mirrored result;
topological face counts need not match when construction differs. Full-symmetric
versus mirrored parity passed: seven valid solids, volume within one part per
million, matching bounds within 1e-5 mm, and all 429 three-axis ray hit counts
and positions within 1e-5 mm. The comparison exited successfully; artifacts:
`build/debug/fuselage-speedup-full-symmetric.log` and `.brep`.

## Build and regression results

The final Debug application, benchmark and focused test targets built successfully.
All six selected CTest suites passed in 240.64 seconds: `former_geometry_tests`,
`servo_tray_geometry_tests`, `fuselage_alignment_tests`, `fuselage_thicken_tests`,
`processing_tests`, and `fuselage_symmetry_tests`. The last suite contains all
seven full-serial versus half-threaded cases listed above. Log:
`build/debug/fuselage-speedup-final-tests.log`.

## Reproduction

From a configured Debug build, run each timed command without other builds or
tests in progress. `FOAM_BENCH_SERIAL` must be absent for these timings.

```powershell
Remove-Item Env:FOAM_BENCH_FULL_SYMMETRIC -ErrorAction SilentlyContinue
Remove-Item Env:FOAM_BENCH_SERIAL -ErrorAction SilentlyContinue
Remove-Item Env:FOAM_BENCH_REFERENCE -ErrorAction SilentlyContinue
$env:FOAM_BENCH_BREP="$PWD/build/debug/fuselage-speedup-optimized.brep"
& build/debug/Debug/fuselage_benchmark.exe build/debug/fuselage-speedup-fixture.foam

$env:FOAM_BENCH_FULL_SYMMETRIC='1'
$env:FOAM_BENCH_REFERENCE="$PWD/build/debug/fuselage-speedup-optimized.brep"
$env:FOAM_BENCH_BREP="$PWD/build/debug/fuselage-speedup-full-symmetric.brep"
& build/debug/Debug/fuselage_benchmark.exe build/debug/fuselage-speedup-fixture.foam
```

The original baseline requires the archived baseline binary or a build of the
baseline revision; the full-symmetric switch uses the new profile semantics and
threading, so it is a parity comparison, not the original baseline.

## Display smoke check

Loaded the final optimized BREP in the application's OCCT viewport through
`former_tests --preview-brep`, without regenerating geometry. The check exited
successfully, and the captured isometric shaded view was visually inspected:
the fuselage outline, top opening and visible internal material rendered without
missing exterior faces. Capture: `build/debug/fuselage-speedup-optimized.png`.
This is a display smoke check; dimensional parity is established above.

The rebuilt Debug application was launched after benchmarking and GUI validation.
