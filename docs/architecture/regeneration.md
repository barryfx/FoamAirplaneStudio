# Background model regeneration

`processing::BackgroundJob<Result>` owns a component operation, immutable captured
inputs, stop source, copied progress queue and future result. It is independent
of Qt and wing data so fuselage and stabilizer jobs can reuse it. `MainWindow`
polls every 40 ms; only this GUI thread touches widgets or displays OCCT shapes.
The current application has one active component job at a time.

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
indicators. The application always supplies its job token. Project file version
8 is unchanged: jobs, cancellation flags and generated meshes are transient.
