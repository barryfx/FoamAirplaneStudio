# Hinge performance validation — 2026-09-14

Windows Debug, Intel Core Ultra 9 185H (16 cores / 22 logical processors),
OCCT 8.0.0, Qt 6.11.1. Removed redundant Build calls after the two-shape
Cut/Common constructors, which already perform the Boolean operation.

Three fresh copies of each identical half-wing BREP input were measured.
Timing includes separation, hinge sampling, bevel cuts and internal validation;
it excludes loft creation, disk I/O and external parity checks. End clearance
is explicitly zero for this comparison so the intentional new clearance does
not invalidate parity. These are synthetic fixtures, not the user's project;
whole-app speedup depends on lofting and rendering costs as well.

| Fixture | Baseline trials (ms) | Optimized trials (ms) | Median reduction |
| --- | --- | --- | --- |
| rectangular-tape | 8854.425, 8943.875, 9056.453 | 5153.026, 5097.911, 5120.892 | 42.7% |
| rectangular-standard | 10013.377, 9865.086, 10011.899 | 5706.561, 5598.387, 5256.925 | 44.1% |
| tapered-mixed | 35101.683, 35375.222, 35207.049 | 18876.355, 19453.472, 19388.039 | 44.9% |

Both rectangular results have byte-identical BREP serialization and identical
volumes. The tapered result differs only in 30 vertex tolerance values, all
smaller than baseline; every geometry coordinate and remaining serialized
entry is identical, with unchanged volume and three valid solids. The benchmark
accepts only identical serialization or this strictly checked tolerance tightening;
other differences require symmetric-difference Boolean and volume checks.
An earlier full Boolean parity attempt was stopped because comparing coincident
loft faces was excessively slow; the strict serialization comparison completed.

Reproduce with the manual `control_surface_benchmark OUTPUT_DIR [BASELINE_DIR]`
target (not registered with CTest). A baseline run stores inputs and results;
pass that directory to an optimized run using a fresh output directory. Local
raw evidence is in build/debug/hinge-baseline and hinge-parity-verified.
The preserved baseline ControlSurfaceCut.cpp has SHA256
18229E80BBBB0F21362EDA3C1AE0854332D5D1EFDF170CF84E7893966D1B7E84.
To rerun the original implementation, use its original six-argument benchmark
call; current benchmark passes explicit zero clearance as a seventh argument.

The new 1/16-inch end clearances use an inset extraction prism and the same
number of Boolean operations. No claim is made that their runtime is identical
to the zero-clearance measurements above.

Final validation with production clearance enabled: Debug build succeeded;
project_tests, control_surface_editor_tests, control_surface_tests and
station_workflow_tests all passed (4/4, 49.63 seconds). The workflow capture
build/debug/control-end-clearance.png was visually inspected: the wing and
separate controls render successfully. Exact narrow gap widths are verified
by solid classification and volume tests rather than screen pixels.
Rebuilt Debug app launched successfully; the prior user instance was preserved.
