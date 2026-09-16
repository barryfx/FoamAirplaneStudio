# ADR-0002: Reusable layered sketch editor
Status: Accepted
Date: 2026-09-13

## Context
Wing outlines need interactive lines and interpolating splines. Other component
editors will need the same point selection, snapping and dragging behavior.

## Decision
Use a reusable SketchEditor owned by the 2D view, with independent sketch layers,
shared point indices within each layer, and line/spline records referencing those
points. WingOutlinePanel supplies the wing-specific UI and connectivity check.
OCCT interpolates nonperiodic splines through the selected points; sampled paths
are display representations, not the source geometry. No new dependency is added.
Render sketches in the view foreground so rebuilding reference scene items cannot
destroy them. Input uses scene coordinates and an eight-pixel screen tolerance.
Snapping across layers copies coordinates; identities remain local so edits never
change another panel. Only the active layer can be dragged or extended.

## Alternatives Considered
Wing-specific mouse handling would duplicate behavior in future editors. Editable
scene items as the only model would couple geometry ownership to image reloads.
Polyline approximations as stored geometry would discard spline intent.

## Consequences
Tools remain selected after curve completion. Escape finishes a spline or discards
a lone pending point. Tool/tab/workspace changes finish valid pending curves.
The panel count is 1–100; reducing it removes the highest numbered outlines.
This decision initially left persistence, undo, aircraft-dimension calibration,
and manufacturing validity checks for later work. Wing calibration was added in
ADR-0005 and sketch persistence in ADR-0006; undo remains future work.

## Validation
SketchEditorTests exercises button toggling, point reuse at changed zoom, fitted
splines, Escape, shared-junction dragging, panel isolation, reference rebuilds,
physical-scale toggling, count changes and reset. Existing viewport, reference and
wing-workflow regressions are also run on Windows Debug.
