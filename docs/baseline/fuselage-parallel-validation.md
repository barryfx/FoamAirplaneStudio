# Shared Fuselage parallel generation validation

Date: 2026-09-19. Branch Assembly. Targeted Fuselage generation benchmarks and
correctness checks were explicitly authorized by the user. No Wing generation
benchmarks or tests are included.

Windows/MSVC Debug application and OCCT Debug libraries, Intel Core Ultra 9 185H,
22 logical processors, approximately 32 GiB RAM. Frozen input:
`build/debug/gentlelady-retainers-fixture.foam`, SHA256
`64F5328181B58C47EA799613C474F775B4F469A29DAA0333213AB4C239A1241B`.

The pre-change binary was retained as `fuselage_parallel_baseline.exe` in the
Debug output directory. Its build completed in 1682.94 seconds, yielding seven
valid solids with volume 891277.5297694321 mm3. Retaining rails (construction,
fusion and validation) occupied 707.095 seconds, from 514.815 to 1221.91 seconds.
The final mesh occupied approximately 36.89 seconds. The unchanged fixture includes
walls, a servo tray, three formers, retaining rails, a side cut and alignment pins.

The baseline generated `build/debug/fuselage-parallel-baseline.brep` for parity
comparison. The full generation timer includes meshing; reference serialization,
post-build validation and comparison are outside that timer. No competing build
or test workload ran during the timed baseline generation section.

Implementation: independent section offsets use up to four workers, with private
OCCT topology and ordered result collection. Per-operation OCCT parallel flags
cover Boolean operations, validity analysis and meshing. Global kernel settings,
feature dimensions, sampling, fuzzy tolerances and operation order are unchanged.
`ProcessingControl::parallel=false` retains a serial comparison mode; the benchmark
selects it when `FOAM_BENCH_SERIAL` is set.

## Focused validation

The Debug app and all focused targets compiled successfully. Passed checks:
`former_tests`, `former_geometry_tests`, `fuselage_cut_tests`,
`fuselage_alignment_tests`, `servo_tray_geometry_tests`, `processing_tests`,
`fuselage_profile_tests`, and `fuselage_thicken_tests`. The wall suite also passed
with `--serial`. Coverage includes numeric rail/support dimensions and insert
clearance, cuts, pin/socket placement/material change, section dimensions, open
and closed ends, varying walls, cache reuse, and cancellation before wall offsets.

A pre-existing closed-end assertion probed Y=0, now the boundary between the two
main-body halves. It failed identically in serial and parallel modes. The material
probe now uses Y=1 inside the cap instead of the seam. The test targets' save-format
assertions were updated from the older version 20 to the current version 23.
Some untimed regression checks overlapped compilation; their elapsed times are
not used as performance evidence. No builds or other tests overlap the timed
parallel benchmark generation.

## Full-fixture comparison

| Timed stage | Original serial (s) | Parallel (s) |
| --- | ---: | ---: |
| Complete generation including mesh | 1682.94 | 928.19 |
| Retaining rails: construction, fusion, validation | 707.095 | 254.187 |
| Final display mesh | 36.89 | 41.058 |

The observed complete build took 44.85% less time (1.81 times as fast), saving
754.75 seconds. This is one run of each binary on one fixture in Debug, not a
statistical estimate or a claim for Release builds or complete Assembly timing.
Later pin-location and meshing stages did not improve; the main measured gain
was in accessory and rail operations. Internal parallelism can increase peak
memory use and competes with other Assembly component work.

The optimized result matched the baseline volume and all six bounds at the
printed precision, with 8974 faces in each. Reference parity passed for valid
solid count, volume within one part per million, bounds within 0.00001 mm, and
429 rays across three axes with matching hit counts and positions within
0.00001 mm. These sampled checks complement the focused feature assertions;
they do not prove exact surface equivalence for every possible input.

Logs: `build/debug/fuselage-parallel-baseline.log` and
`build/debug/fuselage-parallel-optimized.log`. Reference/result shapes are
`build/debug/fuselage-parallel-baseline.brep` and
`build/debug/fuselage-parallel-optimized.brep`. These local build artifacts are
not committed. Reproduce with `fuselage_benchmark.exe <fixture.foam>`, setting
`FOAM_BENCH_REFERENCE` to the baseline BREP and `FOAM_BENCH_BREP` to the output
BREP; set `FOAM_BENCH_SERIAL=1` for the new binary's serial comparison path.

The benchmark exited successfully with seven valid solids. A BREP-only viewport
smoke check saved `build/debug/fuselage-parallel-optimized.png`; visual inspection
confirmed the complete shaded fuselage displayed. This exterior view does not
expose internal features. The rebuilt Debug application was not launched, per
the user's final instruction. `git diff --check` passed.
