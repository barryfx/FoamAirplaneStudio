# ADR-0056: Straight side-view stiffeners at constant inward depth
Status: Accepted
Date: 2026-10-04

## Context
Following the local height fraction at every fuselage section bent the groove in
Side View. The user requested a line between the Start and Stop height fractions,
then clarified that depth must remain constant and explicitly accepted lateral
curvature visible in Top View.

## Decision
For each stiffener, determine its height fraction only at Start and Stop:
50% for one, thirds for two, quarters for three. Linearly interpolate X/Z
between those points, then intersect the external section at that Z to obtain Y.
Use the entered inward depth at every section: Strip Height or half the Round
diameter. Keep Y/Z cross sections, shared cutter/stock routing, right-half cutting,
mirroring, parallel Booleans, cancellation and disconnected-solid rejection.

Use bounded ruled lofts, with the initial 65 positions plus wall sections and
adaptive lateral subdivision. Test lateral interpolation at quarter points and
the midpoint against min(0.01 mm, 1% of depth). Limit recursive subdivision and
reject an excessively difficult profile rather than silently loosening the fit.
Probe actual wall material along the new path, not the old local height fraction.

## Alternatives Considered
A completely straight 3D axis has varying depth on a curved side. The user chose
straightness in Side View with constant inward depth instead. Following local
height fractions would retain the rejected vertical bends. A global smooth loft
can overshoot despite valid input points (ADR-0055). Depth normal to the skin
would change the existing meaning of inward stock dimensions and is not used.

## Consequences
Grooves are straight in Side View and may curve in Top View. Nominal inward Y
depth stays constant within the sampled surface approximation tolerance. The
existing bend limit applies to lateral curvature; it is not a stock bend-radius
model. A line can leave a pinched or strongly curved side outline despite valid
endpoints; generation reports the affected percentage and publishes no new stock.
No project-format fields or defaults change. Existing projects regenerate with
the revised placement. Weight and balance uses the same new route.

## Validation
Route tests cover one and three stiffeners, strips and round rods, straight
side-view placement, constant depth between samples, stock centroids/volume,
a changing side profile checked against analytic surface intersections, and
rejection where the endpoint line leaves the outline. Relevant generation,
symmetry, cancellation, cache and panel tests are recorded in
`../baseline/straight-side-stiffener-validation.md`.
