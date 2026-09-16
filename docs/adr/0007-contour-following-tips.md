# ADR-0007: Preserve the planform when forming wing tips
Status: Accepted
Date: 2026-09-13

## Context
A single global bevel clipping plane removed portions of rounded and swept
wing outlines. Sparse uniform span sections also flattened curved planform detail.

## Decision
Keep the section-loft representation and include every sampled outline vertex's
span in the section schedule, in addition to station and uniform sections.
For each airfoil point, intersect an outboard ray with the sampled outline to
measure the available span at that chord location. For a flat-bottom tip retain
the lower ordinate and compress the upper surface toward it as available span
falls below local thickness; flat-top performs the opposite operation. The
thickness limit equals available span (nominal 45-degree bevel). Plan coordinates
are unchanged. Blunt tips retain both surfaces. A 0.01% residual local thickness
avoids degenerate section wires at a flat outer edge.

This supersedes ADR-0005's global clipping-plane tip construction. The project
file format and saved tip selection remain unchanged; solids are regenerated.

## Alternatives Considered
A separate loft with rotating profiles is possible, but requires an additional
rule identifying the tip shoulder and handling profile correspondence around
arbitrary curves. The contour-following height construction preserves the given
planform with the current station model and avoids a Boolean seam between bodies.
The previous global cutting plane cannot preserve arbitrary tip contours.

## Consequences
The tip follows the traced contour instead of one global plane. Flat top/bottom
refer to retaining the corresponding airfoil surface; camber need not be globally
planar. More outline detail creates more loft sections. The sampled section loft
remains an approximation between samples; unusually folded outlines and crossing
stations remain unsupported. No background job system or dependency is introduced.

## Validation
WingSolidTests checks vertical intersection just inside rectangular and rounded
tip outlines, all three styles, validity, mirroring, volume and scaling. Workflow
and project tests cover processing cursor lifetime and Reference gating after New.
