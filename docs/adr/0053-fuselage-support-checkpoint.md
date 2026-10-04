# ADR-0053: Cache the fuselage after support rail generation
Status: Accepted
Date: 2026-10-04

## Context
Stiffener, hole and cut edits previously rebuilt the outer loft, cavity, tray,
formers and retaining rails even when none of their inputs changed.

## Decision
Retain one immutable in-memory checkpoint with each completed fuselage result,
immediately after retaining rails and before stiffeners. The builder accepts an
optional previous checkpoint and compares an exact binary encoding of upstream
geometry inputs. Both direct Fuselage generation and Assembly/Inspect preparation
pass the previous result's checkpoint. New/Open clears the owning model.

The checkpoint owns the supported body, whole cavity, removable inserts, wall
sections, wall station values and projection transforms. Hollowing always retains
the cavity so adding the first downstream feature does not need a new loft.
Resumed and initial downstream builds deep-copy checkpoint geometry before any
Boolean or meshing operation. Mutable OCCT topology is never exposed by the
opaque checkpoint. There is no shared global cache and no project-format change.

Only successful complete jobs publish a new checkpoint through the existing
epoch/fingerprint checks. Failure or cancellation preserves the previous cache.
This deliberately does not retain checkpoints from an unsuccessful first build.

## Alternatives Considered
- A global mutable cache: complicates ownership, cancellation and concurrent jobs.
- Disk serialization: adds format/version management and is unnecessary for edits.
- A checkpoint after every operation: increases memory and dependency complexity.

## Consequences
Downstream edits skip preparation through rails but still perform stiffeners,
reflection, holes, cuts, alignment and meshing. Upstream edits rebuild preparation.
The last successful checkpoint remains available through failed downstream edits.
Memory usage increases by one unmeshed intermediate model; deep copying adds cost
to each build. Full rebuild remains available by omitting the checkpoint.

## Validation
`fuselage_cache_tests` compares fresh and resumed solids, volume, centroid, bounds,
inserts and carbon stock for stiffener, hole and cut edits. It checks upstream
invalidation, cancellation/failure isolation, repeated reuse, and adding a first
hole to a model without inserts. It reports fresh/resumed timings. The complete
CTest suite includes generation and GUI regressions.
Recorded parity, full-suite results and repeated timing samples are in
`../baseline/fuselage-checkpoint.md` (80.5–81.3% lower elapsed time on the small
Debug fixture; not a general Release performance guarantee).
