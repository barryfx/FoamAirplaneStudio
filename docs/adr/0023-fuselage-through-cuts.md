# ADR-0023: View-projected fuselage splitting paths
Status: Accepted
Date: 2026-09-17

## Context
Fuselage hatches and sections require editable open or closed connected cut paths
in Top or Side View, preserved for regeneration and visible outside Cut mode.

## Decision
Use a dedicated two-layer SketchEditor and project each connected path into its
view plane. Extrude analytic Line/OCCT interpolated Spline wires through the body
and split the completed exterior or thickened solid using OCCT Splitter. Preserve
all solids in a compound and conserve material volume. Project version 13 stores
the cut sketch state; older files load empty cuts. Use the existing Fuselage worker
and its own input fingerprint.

## Alternatives Considered
Closed-area subtraction would discard hatch material and prohibit open paths.
Infinite endpoint extensions could cut unintended regions. Finite extruded sheets
follow the user-drawn path and require open ends to reach the body's boundary.
Mesh cutting would lose the existing OCCT solid representation.

## Consequences
Users choose the projection explicitly. Paths in either view remain registered
with their reference outlines. Every cut must separate a body. No kerf or clearance
is inferred; all separated bodies stay in place. Newer files require this reader,
while versions 1-12 remain readable without cuts.

## Validation
Focused geometry and GUI tests cover mixed curves, both projections, closed
paths, hollow-body splits, volume conservation, invalid/nonseparating paths,
cancellation, persistent drafts, visible overlays and Fuselage-only caching.
Results are recorded in the baseline Cut validation note.
