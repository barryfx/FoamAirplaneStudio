# ADR-0036: Per-former Side View rotation
Status: Accepted
Date: 2026-09-19

## Context
Formers need independent angular placement while retaining cavity fit, thickness,
support rails, editor interaction and reproducible saved geometry.

## Decision
Store unrotated rectangular masks plus per-former degree angles in format 24.
Rotate about each mask center in Side View, with negative counter-clockwise.
Older projects load zero angles. Older applications reject the new version to
avoid silently dropping rotation. Rotate slabs before cavity fitting and rotate
their retaining slabs around the same axis. Use oriented mask overlap checks in
the editor, persistence validation and builder.

## Alternatives Considered
Storing arbitrary polygons complicates thickness and height editing. Rotating
already-fitted inserts would allow skin penetration and leave retainers behind.
Keeping the previous format version would let older readers silently lose angles.

## Consequences
Angles participate in Fuselage/Assembly cache invalidation. Rectangles remain
compatible with older project inputs; new output requires an updated reader.
Rotation retains the existing conservative Side View mask collision policy.

## Validation
Editor, persistence, direction, volume, cavity clearance, retainer alignment and
cache invalidation checks are included in FormerTests. See
../baseline/former-rotation-validation.md for build and suite results.
