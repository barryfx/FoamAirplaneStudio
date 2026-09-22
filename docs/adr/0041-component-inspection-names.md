# ADR-0041: Component inspection and persistent names
Status: Accepted
Date: 2026-09-21

## Context
Inspect must hide individual solids and rename them for Export without changing
model geometry. Generated shapes are session-only and cannot provide persistent
OCCT handles across project reloads.

## Decision
Reuse the export solid catalog for Inspect. Attach source identifiers and solid
ordinals independent of display names. Fuselage source pieces retain identifiers
through Assembly cuts; former identifiers use source mask indices, independent
of nose-to-tail display order. Other model solids use component category/ordinal.
Format 27 stores an optional-name map; versions 1–26 initialize an empty map.
Visibility is transient and defaults checked, independent of export selection.

STEP output uses the saved project basename and user-defined names label its
parts. Unnamed projects use Untitled. Validate component names for filesystem use
and reject output collisions. Inspect reuses the Assembly preparation job with a readiness mask to regenerate
only changed/missing components having complete definitions. Unchanged component
caches are retained; the usual cancellation, progress and stale-publication guards
apply. Renaming and visibility changes do not trigger CAD conversion or regeneration.

## Alternatives Considered
Mutable labels cannot identify renamed parts. OCCT object pointers cannot survive
reloads. Geometric topology matching across arbitrary modeling changes would be
a new naming subsystem beyond this feature's scope.

## Consequences
Names survive reload/regeneration with the same source and solid order. Changes
that insert/delete/reorder former masks or split/reorder solids can change ordinal
identity; users should review names afterward. Generated geometry remains absent
from saved files. Existing files remain readable and are upgraded only on Save.

## Validation
Focused synthetic-model GUI and export checks cover selection, visibility, names,
camera behavior, project persistence, and renamed STEP/DXF/STL output. A focused Inspect regeneration check covers dirty input, unchanged-cache reuse,
cancellation and retry with one small Wing; broad generation suites are excluded.
