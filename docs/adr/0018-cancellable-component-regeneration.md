# ADR-0018: Cancellable component regeneration
Status: Accepted
Date: 2026-09-16

## Context
Wing Boolean operations can take minutes in Debug. Panel source data are independent after global scale, dihedral miters and lightening ranges are prepared. Future fuselage and stabilizer geometry needs the same background-processing lifecycle. The user requires optimization before panel concurrency and only Cancel to remain interactive during a build.

## Decision
Mesh the right half once and mirror a transformed copy of its triangulation. Normalize copied triangle winding against CAD surface normals before display; OCCT 8 can reverse both the face and its cached triangles. Measurements rejected batching spar tools and oriented bounding-box rejection on this workload. Retain topology and volume validation, section resolution, clearances and wall sampling. Measure sequential performance and geometry parity before enabling panel concurrency.

Use owned background component jobs with immutable input snapshots, explicit stop tokens and queued progress/result delivery. GUI polling alone touches widgets and the OCCT viewer. Each wing panel is an independent task; a bounded worker group prevents creating 100 CAD threads for 100 panels. Assemble results in panel order after workers finish, then mesh the right half and mirror it with its triangulation. The generic component-job lifecycle and indexed task runner can be reused for fuselage and stabilizers.

Disable menus, toolbars, data editors and viewport controls during regeneration; leave Cancel available at the bottom of the data panel. Cancellation requests cooperative interruption through OCCT progress ranges and checkpoints. Never terminate worker threads. Report panel-qualified status messages. Publish only a complete, current result. Preserve the last displayed model on cancellation and retain the camera when publishing its replacement. Closing requests cancellation and waits asynchronously for safe completion before proceeding.

## Alternatives Considered
- GUI-thread event pumping: reentrant edits and unresponsive long kernel calls.
- One unbounded thread per panel: excessive memory and CPU oversubscription.
- Forced termination: unsafe OCCT state and resource lifetimes.
- New process or third-party scheduler: unnecessary dependencies for independent geometry tasks.

## Consequences
Kernel operations without interruption hooks may finish their current call before stopping. No partial panel results become the project model. Source data and file format remain unchanged. Modal file operations continue using their existing synchronous processing scope. Worker-owned shapes must not enter the renderer until all modifying geometry work has finished.

## Validation
See docs/baseline/regeneration-validation.md for sequential measurements, parity checks, parallel timing, cancellation and GUI lifecycle coverage.
