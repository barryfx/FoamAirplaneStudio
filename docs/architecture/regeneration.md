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
samples. Uninterruptible validation, integration, unification and transform calls
can delay cancellation until that call returns. There is no thread termination.

During regeneration all editing widgets, menus/actions/shortcuts, toolbars and
viewport interaction are disabled. Cancel stays at the bottom of the data panel.
After a request it reads Cancelling and is disabled until workers finish. Status
messages identify the wing panel producing them, followed by assembly/display or
completion/failure/cancellation. Progress itself never changes saved project data.

A cancelled job never publishes its result, even if completion races the request.
The previous model remains visible and the source inputs are unchanged. Re-enter
3D View to retry, or edit a geometry input. Toolbar navigation alone does not
restart a cancelled job. Successful replacement retains the camera; only the first model
fits automatically. Failures restore the controls and report the error. A project
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

Fuselage generation now fuses former retaining rails before user cuts, splits
only the largest post-cut main body at Y=0, and finally appends whole removable
tray/former inserts. See ADR-0029 for dimensions and cut-out classification.

After the main fuselage centre split, four alignment pins and matching deeper
sockets are derived from the retained seam material and local station walls.
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
See fuselage-thickness.md and ADR-0044. The fuselage is still generated as a full
body before splitting; building one half and mirroring it is not implemented.
