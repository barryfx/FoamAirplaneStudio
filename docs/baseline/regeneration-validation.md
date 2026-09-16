# Cancellable panel regeneration validation

Date: 2026-09-16. Windows Debug, MSVC, Qt 6.11.1, OCCT 8.0.0.
Machine: Intel Core Ultra 9 185H, 16 cores / 22 logical processors.

## Fixture and method
Read-only `build/debug/lightening-smoke.foam`: two dihedral panels, surface spar
grooves and lightening. SHA-256:
`6EC6337B476B9672396291229FC8B6D5849EF059296ABA4B050934B920F47219`.
The baseline executable and BREP were saved before changes under build/debug.
No DesignRC files or source project fixtures were modified.

`regeneration_benchmark FILE.foam` times generation only, excluding BREP I/O,
volume/bounds measurements and parity checking. It defaults to one panel worker.
Set FOAM_BENCH_PARALLEL=1 for the bounded concurrent path; FOAM_BENCH_REFERENCE
selects the baseline BREP, and FOAM_BENCH_BREP saves a result. Progress timestamps
separate modeling from display meshing. Each run is a single Debug sample, not a
statistical or Release performance claim. Other tests started only after the
sequential model timing had completed.

## Sequential optimization
Original: 410.753 s overall. Mirroring 377.568-377.980 s; display meshing
377.980-410.753 s (32.773 s).
With right-half mesh reuse: 496.626 s overall. Meshing 480.305-496.020 s
(15.715 s); mirror plus mesh copy 496.020-496.626 s (0.606 s).

The duplicate display tessellation is removed (about 52% lower meshing time in
these samples). Boolean stages varied substantially; these runs do NOT establish
an overall sequential speedup. Batched spar cuts and oriented bounding boxes
were tried and rejected after slow stage timings. Final spar Boolean ordering,
geometry sampling and validation tolerances are retained.

Sequential parity passed: eight valid solids, equal reported volume
2806150.962965255 mm^3, equal bounds, and 429 off-grid rays along three axes with
intersection differences below 0.00001 mm. Volume tolerance was relative 1e-6;
bounds tolerance was 0.00001 mm. The resulting BREP remains in build/debug.

## Automated and GUI validation
- processing_tests: bounded simultaneous tasks, first-error propagation with
  sibling cancellation/join, copied progress strings, cancellation and OCCT
  indicator/range lifetime.
- project_tests: async restoration/camera preservation; all menus/actions,
  toolbars, editors and viewport disabled during processing; Cancel active at
  the bottom; cancellation retains inputs/revision/camera and restores controls;
  close waits safely; programmatic project replacement rejects stale results.
- Updated station_workflow_tests waits for completed background jobs rather than
  assuming synchronous generation.
- Screenshot regeneration-cancel-panel.png visually confirms the disabled UI and
  enabled bottom Cancel button. Its QWidget capture excludes the native 3D child.

Geometry regressions passed: wing_solid_tests 158.37 s, spar_tests 91.83 s,
control_surface_tests 18.42 s, lightening_tests 107.39 s. These correctness runs
used concurrent test processes and are not benchmark measurements.
Project tests including the failure-recovery case passed in 112.49 s;
station_workflow_tests passed in 99.76 s; processing_tests passed in 0.07 s.
The final cancellation-navigation checks and parallel benchmark are recorded below.


## Concurrent result and mirrored display validation
Two distinct panel worker threads completed the same fixture in 283.902 s,
compared with the original 410.753 s (about 31% less elapsed generation time in
this single Debug sample). Panel 1 completed at 124.490 s, Panel 2 at 266.954 s.
The parallel result passed the same eight-solid, volume, bounds and 429-ray
checks. No correctness tests were running during the measured parallel build.

Visual comparison caught OCCT 8 reversing cached triangle winding along with
face orientation on reflection. The renderer then exposed internal pocket faces.
`alignMeshOrientation` tests winding against parametric CAD surface normals and
corrects only cached triangles, without changing nodes, UVs or CAD geometry.
Normal caches are discarded for recalculation from the transformed surfaces.
The full cached result needed 3198 face corrections in 0.054 s; the original
baseline needed zero. Both contained 113318 checked triangles and the same two
faces without cached triangulation. The orientation test uses area-weighted
face agreement because tiny tip facets can straddle surface singularities.

The corrected rendered PNG and the original baseline PNG are byte-identical:
SHA-256 `598A9DA6CBC66D4EF3FF291167862B7478B8879B3E68C0E3FD10288972D85B6D`.
Files: regeneration-wing-baseline.png and regeneration-wing-fixed.png in
build/debug. mesh_orientation_tests also generates a curved dihedral wing,
checks its mirrored meshes, deliberately reverses a face, and verifies repair
and idempotence. The 283.902 s model measurement predates this 0.054 s cache-only
correction; it does not include that measured postprocessing cost.

The cancellation/navigation adjustment passed project_tests (111.83 s) and
station_workflow_tests (110.08 s): toolbar navigation does not restart a cancelled
job, and re-entering 3D or changing an input can retry it. An input-fingerprint
check also discards a result if a late data-entry commit changes the snapshot.

Final validation on 2026-09-16: the full Windows Debug build succeeded. All eight
relevant CTest suites passed: project_tests, station_workflow_tests,
processing_tests, mesh_orientation_tests, wing_solid_tests, spar_tests,
lightening_tests and control_surface_tests (447.95 s total with two test jobs).
The rebuilt Debug application was launched and its FoamAirplaneStudio window
was confirmed running.
