# ADR-0032: Fuselage half alignment pins
Status: Accepted
Date: 2026-09-18

## Context
The left/right fuselage halves need four locating pins on their mating surfaces:
two top and two bottom toward the ends. Pins project 3 mm; matching sockets are
3.5 mm deep. Diameter is 4 mm, capped by local wall thickness.

## Decision
Add pins after the main-body centre split, before appending unchanged cut-out
parts and removable inserts. Pins belong to the negative-Y half and point into
the positive-Y half. Sockets have the same diameter (no extra radial clearance)
and 0.5 mm extra axial depth. A 0.25 mm embedded root joins each pin to its half.

Target 15% and 85% of the retained main body's length. Search 5-35% and 65-95%
in 2% increments, closest target first, independently for top and bottom. Use the
uncut supported body to identify the true top/bottom skin, so a missing hatch
region cannot be confused with the opposite wall. Diameter is min(4 mm, smoothly
interpolated nominal wall thickness, local vertical skin span). Solid, unthickened
inputs use a 4 mm nominal cap. Prefer mid-wall within that top/bottom span and adjust within available material when a sloping skin requires it.

Cross-section samples reject obvious unsuitable candidates, followed by exact
OCCT cylinder containment in the post-cut main body. Require support through the
pin root and socket depth, plus 0.1 mm material beyond the socket bottom. Do not
place pins on cut-outs, in cavities, or through outer skin. Keep circular footprints
separate. If four safe locations cannot be found, report the offending end/surface
instead of publishing fewer pins or an invalid model.

Fuse all four pins in one operation and subtract all four sockets in another.
Validate one positive-volume solid per half and the expected volume additions
and removals. Cancellation uses the existing worker control. No new persistent
fields: regeneration derives placement from outlines, thicknesses and cuts.

## Alternatives Considered
Fixed bounding-box positions could land in hatches or narrow tapered regions.
Silently omitting pins would violate the four-pin requirement. Oversized sockets
would introduce an unrequested radial manufacturing clearance. A configurable
pin editor and clearance controls remain outside this change.

## Consequences
A fuselage without sufficient mating material now reports a placement error.
Total material decreases by four socket-bottom clearances; pre-feature splitting
still conserves volume. Pins remain separate from hatches, trays and formers.
The previous rail-performance measurement predates alignment-pin generation.

## Validation
See ../baseline/fuselage-alignment-validation.md for dimension, thin-wall,
variable-wall, hatch, failure, cancellation and regeneration checks.
