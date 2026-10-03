# ADR-0051: Incremental Weight and Balance measurements
Status: Accepted
Date: 2026-10-03

## Context
The aggregate cache avoided repeated calculation only while all geometric inputs
were unchanged. Editing one fiberglass boundary remeasured every patch and foam
component. The user requested individual reuse and a 90% hardware-thread budget.

## Decision
Keep the aggregate cache and add MainWindow-owned, transient caches for each
component/plywood volume and each fiberglass patch area/centroid. Read cached
entries before scheduling, execute only misses, and publish replacements after
successful joining. Retain only the current measurement set, not an unbounded
history. New/Open/Close clears caches. No persistent format changes are needed.

Match immutable OCCT topology and orientation plus numeric placement transforms.
TopLoc datum identity alone is insufficient because repeated placement constructs
equivalent new datums. Rebuilt/cut topology invalidates dependent measurements.
Fiberglass keys additionally encode the patch boundary, wrap/surface selection,
scale, relevant outline/projection frame and placement. Exclude names, material
values, editor selection and unrelated patches. Reordering/deleting patches does
not invalidate surviving measurements; use input order for deterministic results.

Schedule each changed fiberglass patch independently, including multiple patches
on one component. Use `max(1, floor(0.9 * hardware_concurrency))` workers, further
limited by the number of misses. Unknown hardware concurrency falls back to one.
Apply this budget only to Weight and Balance, leaving generation scheduling
unchanged. Disable nested covering-mesh parallelism. Every covering worker owns
its mesh, projection state and visibility classifier; cache publication and GUI
updates stay on the caller thread.

## Alternatives Considered
A single aggregate key loses reuse after small edits. Index-only patch keys fail
after deletion or reordering. Bounds-only solid keys miss actual shape changes.
Persistent CAD cache identities are not portable across project sessions and
would require a larger format change. Unbounded cached history retains obsolete
topology and grows memory use over long editing sessions.

## Consequences
Material-value edits only reweight cached geometric measurements. Geometry edits
invalidate affected components/patches, while other contributions are reused.
Placement changes remeasure the moved component's results. More changed patches
can run concurrently, increasing temporary mesh memory with the worker budget.
The percentage is a thread ceiling, not a guaranteed CPU-utilization percentage.

## Validation
Analytical primitive fixtures compare serial, parallel and cached results and
verify invalidation counts for single edits, topology, placement, scale, metadata,
reordering, deletion and failed calculations. See
`../baseline/weight-balance-cache-validation.md` for timing and scoped test evidence.
