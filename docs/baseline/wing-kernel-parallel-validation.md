# Wing internal parallelism validation

Date: 2026-09-19. Branch Assembly. The user explicitly authorized targeted Wing
generation benchmarks and correctness checks. Windows/MSVC Debug with OCCT Debug,
Intel Core Ultra 9 185H, 22 logical processors, approximately 32 GiB RAM.

## Method

The existing two-panel dihedral/surface-spar/lightening fixture is
`build/debug/lightening-smoke.foam`, SHA256
`6EC6337B476B9672396291229FC8B6D5849EF059296ABA4B050934B920F47219`.
The current pre-change benchmark was rebuilt and preserved as
`build/debug/Debug/wing_kernel_baseline.exe`. Both timed runs use
`FOAM_BENCH_PARALLEL=1`, keeping the existing two concurrent panel workers.
No builds or correctness tests overlap the timed generation runs. The timer
includes generation, mesh preparation and reflection, but excludes BREP writing
and post-build validity/parity checks.

The baseline took 279.545 seconds and produced eight valid solids with volume
2806150.962965255 mm3. Logs and reference geometry are
`build/debug/wing-kernel-baseline.log` and
`build/debug/wing-kernel-baseline.brep`. These are local, uncommitted artifacts.

## Change

Enable per-operation OCCT parallelism in control separation/bevel Booleans, spar
cuts, access splitters, lightening subtraction and tab fusion. Enable parallel
shape analysis in panel/control/spar/pocket validation. Preserve Boolean order,
non-destructive settings, tolerances, sampling, pocket geometry, mesh reuse and
cancellation ranges. No new application worker pool or global kernel setting.

The existing panel scheduler, parallel meshing and parallel pocket fusion remain
unchanged. `FOAM_BENCH_KERNEL_SERIAL=1` disables only the newly enabled kernel
work, by passing `WingBuildOptions::processing.parallel=false` to each panel.
This comparison is separate from `FOAM_BENCH_PARALLEL`, which controls panels.

## Measurements

| Metric | Current baseline (s) | Additional kernel parallelism (s) |
| --- | ---: | ---: |
| Complete generation | 279.545 | 191.562 |
| Panel 1 completion from run start | 120.339 | 67.1311 |
| Panel 2 completion from run start | 263.362 | 175.584 |
| Right-half display meshing | 15.176 | 15.026 |

Observed total runtime decreased 31.47% (1.46 times as fast), saving 87.983
seconds. These are single Debug samples on this fixture, not a statistical or
Release estimate. This measures Wing alone; full Assembly contention is not
benchmarked. More internal parallelism can increase memory use and compete with
other active component workers. Simple wings may see little benefit or overhead.
Meshing was already parallel and remains essentially unchanged in these samples.

Candidate log/result: `build/debug/wing-kernel-optimized.log` and
`build/debug/wing-kernel-optimized.brep`. To reproduce, run
`regeneration_benchmark.exe build/debug/lightening-smoke.foam` with
`FOAM_BENCH_PARALLEL=1`, `FOAM_BENCH_REFERENCE` pointing to the baseline BREP,
and `FOAM_BENCH_BREP` pointing to a new output BREP. Post-timing parity may run
alongside correctness tests; those elapsed times are not performance evidence.

## Correctness and build

The candidate benchmark exited successfully. Both results contain eight valid
solids. Volume and all six bounds match at the printed precision. Comparison
passed for volume within one part per million, bounds within 0.00001 mm, and
429 off-grid rays across three axes with identical hit counts and hit positions
within 0.00001 mm. This sampled comparison does not prove surface equivalence
for every input. The benchmark fixture has no enabled control surfaces; hinge
behavior is covered by the focused control suite rather than its timing result.

All seven selected suites passed: `wing_solid_tests`, `spar_tests`,
`spar_panel_tests`, `control_surface_tests`, `lightening_tests`,
`mesh_orientation_tests`, and `processing_tests`. Coverage includes valid lofts,
separate panels, hinge contact and end clearances, spar dimensions, lightening
wall/crossmember clearance, mirrored mesh winding, and cancellation coordination.
CTest ran two tests concurrently after timed generation finished; total elapsed
time was 173.98 seconds. Log: `build/wing-kernel-tests.log`.

The Debug application and selected targets built successfully
(`build/wing-kernel-build.log`). `git diff --check` passed. The application was
not launched, following the user's instruction to leave it closed.
