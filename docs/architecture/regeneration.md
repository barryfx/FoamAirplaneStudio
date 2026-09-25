# Background model regeneration

`processing::BackgroundJob<Result>` owns a component operation, immutable captured
inputs, stop source, copied progress queue and future result. It is independent
of Qt and wing data so fuselage and stabilizer jobs can reuse it. `MainWindow`
polls every 40 ms; only this GUI thread touches widgets or displays OCCT shapes.
Wing, Fuselage and each stabilizer own separate jobs, input fingerprints and cached shapes.
The editing lock permits one active operation at a time (Assembly coordinates multiple component workers); individual component jobs run on their own
worker, and Fuselage never calls the Wing builder. Switching components displays
the cached shape. Wing regenerates only when Wing geometry inputs change.
Stabilizer jobs use the same lock, progress queue, cancellation and publication checks; see stabilizer-solids.md.

`processing::runIndexedTasks` assigns independent tasks to at most four workers,
further limited by hardware concurrency and task count. Each wing panel has its
own input data, local OCCT shapes and result slot. Workers never mutate shared
CAD geometry. An error cancels sibling tasks and joins them before propagating
the first failure. Completion order does not change panel order, cumulative
dihedral placement, mirroring, or the number/order of resulting solids.

The wing coordinator prepares global scale, root axes, dihedral miters and global
lightening bays, builds the panel tasks, then assembles their complete results.
It meshes the right half once and copies/transforms the triangulation during
reflection. A surface-normal check corrects copied triangle winding before the
renderer applies face orientation, and discards copied cached normals so they
are recomputed from the transformed surfaces. This avoids inward-facing mirrored
skins with OCCT 8 while retaining mesh reuse. Geometry, clearances, wall sampling
and validation tolerances remain
unchanged. Failed Boolean batching experiments were not retained.

`geometry::ProcessingControl` passes stop tokens explicitly, checks safe boundaries
and supplies OCCT progress indicators for lofts, Booleans and meshing. Each kernel
call has its own indicator and borrowed progress range; an RAII holder retains
the indicator until the range dies. Surface sampling checks cancellation between
samples. Fuselage mass integration and bounds traversal check between faces;
pin ray intersections check between faces, lazily loading only face intersectors
whose X/Y bounds meet the ray. Oriented crossings provide material intervals;
the final cylinder/stock Boolean still certifies clearance. Whole-solid point
classifiers are avoided because their eager face caches made cancellation cleanup
very expensive. Wall offsets and their fallback check between sections
and polygon passes. Validation, unification, transforms, one planar offset, and
individual face-intersector initialization calls without a progress API can still
delay cancellation until that kernel call returns. Checks bracket those calls;
there is no unsafe thread termination or detached geometry work.

During regeneration all editing widgets, menus/actions/shortcuts, toolbars and
viewport interaction are disabled. Cancel stays at the bottom of the data panel.
After a request it reads Cancelling and is disabled until workers finish. Status
messages identify the wing panel producing them, followed by assembly/display or
completion/failure/cancellation. Progress itself never changes saved project data.

A cancelled job never publishes its result, even if completion races the request.
The previous model remains visible and the source inputs are unchanged. Re-enter
3D View to retry, or edit a geometry input. Toolbar navigation alone does not
restart a cancelled job. Successful replacement retains the camera; only the first model
fits automatically. Failures restore the controls and report the error. A failed
Fuselage attempt remains suppressed during routine UI refreshes, but explicitly
returning from 2D to 3D clears its attempt marker and retries unchanged inputs.
This also applies to worker startup and display failures. A project
epoch and input-fingerprint check prevent stale results from being published after
programmatic restoration or a late data-entry commit.
Normal New/Open/Save/Close Project actions are disabled during processing. Window
close requests cancellation, then resumes the usual unsaved-data prompt once the
job has stopped. Destruction joins as a final lifetime safeguard.

`WingBuildOptions::maxPanelThreads=1` provides a sequential validation/benchmark
path. With no external cancellation token, it avoids unnecessary kernel progress
indicators. The application always supplies its job token. Jobs and cancellation flags remain transient. Format 23 keeps generated shapes and meshes in memory only. Version-22 embedded geometry is ignored; explicit 3D entry after Open rebuilds models.

Complete Fuselage outlines/profiles suffice to enter 3D. Missing wall defaults are
initialized before snapshot capture, independent of optional tab visits. Cut, tray
and former operations run only when their saved geometry exists. Definition readiness
also enables Horiz Stab and Vert Stab navigation; each generates from its own outline and single airfoil.

Fuselage outer/cavity lofts merge coincident faces and collinear edges before
validation and Booleans (FuselageTopology.h). Kernel tolerances and shape sampling
are unchanged; safe-input mode preserves stage ownership. Unification has explicit
cancellation checks before/after its non-interruptible call. GentleLady timing and
geometry parity evidence: ../baseline/fuselage-readiness-performance.md.

Fuselage generation fuses right-side retaining rails before reflecting the body
into separate halves and applying user cuts. Shared seam faces group cut pieces;
the group with greatest combined volume remains the main body. Only detached
groups are joined into whole cut-outs. Whole removable tray/former inserts are
appended afterwards. See ADR-0029 for dimensions and ADR-0045 for construction.

Four alignment pins and matching deeper sockets are derived from the retained
main halves, their seam material and local station walls.
Cut-out pieces and inserts are appended unchanged. See ADR-0032.

Assembly preparation and seat cutting use the same processing lock and cancellation
controls. Preparation reuses current component caches and generates missing ones.
Cuts deep-copy the placed originals, validate control-surface collisions, and
publish a separate export snapshot only after success. Epoch and source fingerprints
reject stale results; originals remain available for Undo Cuts.

Opening a project always selects 2D View, regardless of its saved viewport, so
opening alone never regenerates models. A saved Assembly workspace opens in
Fuselage/Outline/Side View instead, preserving Assembly placements and cut intent.
Explicitly selecting 3D View or entering Assembly starts model preparation.

Assembly preparation schedules missing Wing, Fuselage, Horiz Stab and Vert Stab
models concurrently through `runIndexedTasks`, bounded to four workers and the
hardware concurrency. Valid caches are reused. Each task owns its input/result
slot; progress is queued under the background job mutex. Failure cancels sibling
tasks and joins them before reporting; cancellation publishes no partial snapshot.
Wing panel concurrency is limited to one while other missing components are
scheduled, avoiding nested worker pools; a lone missing Wing uses its normal panel
parallelism. The GUI remains locked until the complete Assembly result is ready.

## Fuselage internal parallelism

The shared Fuselage builder uses OCCT's per-operation parallel mode for Boolean
operations (servo supports, formers/rails, splitting and alignment features),
shape validation and final meshing. Independent wall-section offsets use the
bounded indexed scheduler; every offset builds private topology from numeric
points. Results are gathered in source order before cavity-continuity checks.
No worker mutates shared OCCT topology and no global OCCT setting is changed.

`ProcessingControl::parallel=false` selects serial execution of these stages for
benchmark comparison. The default is enabled. Geometry sampling, tolerances,
feature dimensions, operation dependencies and validity checks are unchanged.
The final mesh uses the same deflection/angle and now receives the cancellation
indicator. Internal OCCT parallel work uses its scheduler alongside Assembly's
component workers; the four-component limit is not a total kernel thread cap.

## Wing internal parallelism

Wing panels also use per-operation OCCT parallel Booleans for control-surface
separation/bevels, spar grooves, access splits, hollowing cuts and alignment tabs.
Panel, control, spar and pocket validation uses parallel shape analysis. Each
panel still owns its topology; operation ordering and cancellation ranges are
preserved, and no global kernel setting is changed. Existing panel concurrency,
parallel meshing, pocket fusion and mirrored mesh reuse remain in place.

`WingBuildOptions::processing.parallel=false` disables this additional kernel
parallelism for comparisons. It does not disable the older parallel mesh/pocket
fusion stages or panel scheduling. `FOAM_BENCH_KERNEL_SERIAL=1` exposes that
comparison in `regeneration_benchmark`; `FOAM_BENCH_PARALLEL=1` separately enables
the normal bounded panel scheduler. Kernel workers share CPU resources with
panel and Assembly workers, so gains depend on the model and available cores.

Inspect also uses Assembly preparation to refresh missing or changed complete
components. Weight and Balance caches volume/centroid statistics against current
source geometry and Assembly state; changing only parts or densities reuses those
statistics. See inspect.md and weight-and-balance.md.

Before Fuselage lofting, model-only end registration accommodates slightly tilted
flat nose/tail edges and nearby end stations without modifying saved sketches.
See fuselage-thickness.md and ADR-0044. The fuselage now generates its right half and reflects it into a separate left
part. Whole formers/tray use whole-cavity tooling. See ADR-0045 for geometry
semantics and the nonpersistent full-symmetric benchmark comparison path.

Former fitting and per-former retaining-rail construction also use bounded indexed
workers. Each owns deep copies of its cavity/body/insert geometry. Four independent
alignment-pin searches own separate lazy ray caches and CAD copies;
conflicts retry in deterministic placement order. Final pin fusion and socket cuts
run on independent halves. Worker Booleans retain per-operation kernel parallelism; OCCT shares its native
thread pool between concurrent callers. No global pool setting is changed. `ProcessingControl::parallel`
disables the additional workers for serial parity checks. User holes/cuts finish
before pin search, preserving its stock/clearance dependency.

Cancellation validation includes a real GUI Cancel click with four active child
workers, cancellation at each reported fuselage stage, analytic mass/bounds parity,
a stop requested during a many-face integration, and a delayed request during
BabyBuzzard pin-search initialization. The benchmark's optional
`FOAM_BENCH_CANCEL_STAGE` substring and `FOAM_BENCH_CANCEL_DELAY_MS` request a stop
after entering a selected stage and report acknowledgment latency; normal timing
runs leave these variables unset. See `../baseline/fuselage-cancellation-validation.md`.

Stabilizer cut loops are validated before starting any component/Assembly/Inspect workers. Editor warning dialogs block queued generation until editor finalization completes. Worker exceptions end the job and restore controls/cursor; failed stabilizer fingerprints suppress automatic retries until inputs change or the user re-enters 3D. Indexed worker failures request stop on sibling tasks and join them before propagating the error; no partial result is published.
