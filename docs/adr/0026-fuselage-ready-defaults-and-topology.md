# ADR-0026: Complete-definition readiness and compact fuselage topology
Status: Accepted
Date: 2026-09-17

## Context
The user requires generation without visiting Thicken, Cut, Servo Tray or Formers,
and stabilizer navigation as soon as fuselage definitions are complete. GentleLady
has simple profile segments, but the fixed sample correspondence creates many
redundant loft faces that subsequent validation and Boolean operations must process.

## Decision
A complete pair of outlines and assigned closed station profiles enables both
stabilizer workspaces. Generation initializes missing station walls before snapshot
capture, preserving explicit values; optional tab visit history does not gate it.
Stabilizer modeling remains a separate implementation task.

Merge coincident loft faces and collinear edges at existing kernel tolerance
before downstream validation, wall assembly and Booleans. Preserve the 64-point
profile correspondence, guide samples, loft method, thickness law and mesh settings.
Use safe-input mode, retain all existing solid validity checks, and checkpoint
cancellation before and after OCCT's non-interruptible unification call.

## Alternatives Considered
Reducing section/profile samples would change shape approximation. Skipping validity
checks or wall clipping would weaken correctness. Increasing Boolean concurrency
would complicate ownership and peak memory before eliminating redundant work.

## Consequences
Readiness is based on definitions, not whether an optional editor was visited or
a build succeeded. Geometry can still fail validation. Coplanar topology changes
must preserve body count, volume, bounds and sampled surface intersections against
a captured GentleLady baseline. No persistent format or major architecture change.

## Validation
GentleLady improved from 562.595 s to 309.675 s in sequential Debug runs;
solid validity/count, volume, bounds and 429-ray surface parity passed. Four
focused regressions passed, including open/closed-end wall cases. See
../baseline/fuselage-readiness-performance.md for evidence and limitations.
