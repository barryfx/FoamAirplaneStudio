# ADR-0024: Cavity-fitted servo tray and side ledges
Status: Accepted
Date: 2026-09-17

## Context
The user places a rectangular Side View servo tray. Its height is the tray
thickness. A separate tray must touch the inside walls, with integrated supports
5 mm inward and 5 mm below its underside. A later laser exporter needs its plan.

## Decision
Persist one axis-aligned rectangle in version 14. Width/Height fields set physical
dimensions and dragging places the fixed-size rectangle on Side View. Legacy
freehand draft fields remain readable but are cleared when restored.
Reuse the actual inner cavity produced by Thicken. Intersect the tray slab with
it, then derive two 5 mm side bands from a 5 mm cavity slice below the tray. Fuse
only the bands to the body. Apply Cut to that supported body, then append the
separate tray. Return a structured Fuselage model retaining the tray top faces
for future planar-outline export, while preserving the existing shape-only
builder wrapper for callers that do not need part metadata.

## Alternatives Considered
An exterior-width rectangle would penetrate the fuselage walls. An inscribed
constant-width plate would leave gaps along taper. Reconstructing the cavity from
outer dimensions would disagree with station wall offsets. Fusing the tray to
the walls would lose the separate laser-cut part. The initial implementation required opening Thicken. The user subsequently
authorized regeneration with untouched defaults: inserts now initialize missing
station walls and enable hollowing automatically, preserving explicit values.

## Consequences
The cavity determines edge contours throughout the tray thickness. Sloping walls
can require different top/bottom edge contours; only the horizontal top geometry
is retained for the future export workflow in this phase. Full manufacturing
clearance/bevel and export UI remain separate work. The support dimensions are
fixed physical millimetres, regardless of Reference entry units. Existing project
files remain readable; generated solids and planar faces are transient.

## Validation
Focused tests cover dimension entry, fixed-size movement, persistence, version migration, exact 5x5
supports, separate tray/cavity contact, tapered walls, retained top geometry,
cut supports, invalid placements, cancellation, and the independent worker/cache.
Results are recorded in baseline/servo-tray-validation.md.
