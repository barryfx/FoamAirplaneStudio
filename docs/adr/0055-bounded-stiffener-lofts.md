# ADR-0055: Bounded fuselage stiffener lofts
Status: Accepted
Date: 2026-10-04

## Context
The smooth cubic loft in BabyBuzzard36 doubled back and deviated tens of
millimetres from valid route samples. Basic topology validity still passed.
The Boolean retained a small extra solid inside the cutter and generation
failed. See `../baseline/distance-units-stiffener-diagnostics.md` for measured
intersections and the isolated correction experiment.

## Decision
Connect consecutive stiffener sections with a ruled loft for both carbon stock
and groove cutters, for strips and round rods. Keep the existing 65 route
samples, exact-collinearity reduction, sampled clearance checks, bend guard,
parallel Boolean operation and post-cut topology/solid-count rejection.
Equal corresponding strip corners or circle points interpolate linearly in X,
Y and Z between successive sections. This prevents the global smooth loft's
longitudinal foldback and interpolation overshoot by construction.

## Alternatives Considered
Discarding small result solids masks incorrect cuts and can remove intended
foam. Reducing stock dimensions does not correct the interpolated route.
Retaining more of the existing samples does not help this case: none had been
removed. A constrained smooth spline and adaptive sampling may improve route
smoothness later, but require separate error and bend criteria. A single straight
groove would not follow curved fuselages.

## Consequences
Grooves continue following curved booms through short straight spans. There can
be tangent changes at section boundaries; the existing 15-degree sample guard
remains, and this is not a carbon-specific bend-radius guarantee. Stock volume
and centroid use the same bounded path as the cutter. There are no persistent
format or dependency changes. Existing fit checks remain sampled checks against
the fuselage; this change does not claim exact continuous clearance for every
possible body between samples.

## Validation
A small numeric fixture retains the 65 route positions from the failing project,
without embedding its project file or reference image. Tests cut a matching body with strip and
round tools, verify stock volume and one valid solid, and classify material at
quarter, middle and three-quarter positions in every span. They verify that the
groove is open, foam deeper than its specified depth remains, and foam above and
below it remains. Full-suite and actual-project generation results are recorded
in `../baseline/stiffener-bounded-loft-validation.md`.
