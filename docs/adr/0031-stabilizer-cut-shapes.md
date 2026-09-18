# ADR-0031: Stabilizer closed-loop material-removal cuts
Status: Accepted
Date: 2026-09-18

## Context
Stabilizers need multiple editable closed Cut Shapes, including spline loops, and
may separate into additional pieces. The user specified removing inside material.

## Decision
Use one SketchEditor layer per Cut Shape with list/canvas selection and whole-layer
deletion. Persist independent per-component sketches in version 20. Extrude exact
interpolated closed wires into subtraction tools through all model thickness.
Apply mirrored horizontal tools after joining halves, and vertical tools before
rotation. Preserve all remaining valid solids.

## Alternatives Considered
Splitting and retaining the inside body conflicts with requested material removal.
Cutting before centerline joining prevents cuts that intentionally separate halves.
Sampled polygon tools approximate splines unnecessarily and add many Boolean faces.

## Consequences
Existing files remain readable. Incomplete drawings remain saveable and editable,
but warn on mode exit and block generation. Body count may increase after cuts.
No new dependencies or major geometry architecture changes are required.

## Validation
Stabilizer-only tests cover closed line/spline tools, removed volume, multiple
remaining solids, incomplete-loop errors, multiple-shape selection/deletion,
Save/Open, migration, worker generation and Cancel visibility.
