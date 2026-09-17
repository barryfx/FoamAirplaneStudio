# ADR-0019: Independent fuselage outline views
Status: Accepted
Date: 2026-09-16

## Context
Fuselage Outline needs Top and Side sketches with Wing's editing behavior,
closed-loop checks, readiness for Profile Stations and complete project persistence.

## Decision
Reuse SketchEditor through a separately owned PlanViewport editor with two fixed
layers (Top, Side), leaving Wing and airfoil models independent. Extract the existing
airfoil closed-boundary check into a shared sketch utility without changing its
semantics. The Fuselage panel owns mutually locked view buttons and Line/Spline
controls. Leaving Outline warns by view name but preserves incomplete sketches.
Two closed nonzero-area loops enable Profile Stations without selecting it.

Extend the versioned project snapshot to version 9 with fuselageOutline and
ui.fuselageView. Retain readers for versions 1-8, initializing empty fuselage views.
Include geometry/drafts in dirty checking and preserve idle navigation on explicit
Save only. No Wing generation inputs or geometry algorithms change.

## Alternatives Considered
Reusing Wing layers would mix component ownership and readiness. Duplicating mouse
handling would diverge from Wing behavior. Omitting persistence would lose users'
new outlines. Writing extra fields under version 8 would let older writers silently
discard them.

## Consequences
Older files remain readable; older applications reject new version-9 files. Original
files are unchanged until Save. This implements outline editing only; Profile Stations
and downstream fuselage modeling remain future work. Boundary validation shares the
existing finite display sampling and checks connectivity/area, not manufacturing.

## Validation
FuselageOutlineTests covers interaction, view isolation, closure, named exit warnings,
readiness, drafts, migration and lifecycle. Existing project, airfoil, sketch, viewport
and Wing workflow tests remain applicable. Execution evidence is recorded separately
in docs/baseline/fuselage-outline-validation.md.
