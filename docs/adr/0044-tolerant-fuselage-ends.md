# ADR-0044: Tolerant fuselage end registration
Status: Accepted
Date: 2026-09-22

## Context
Hand-traced end edges are seldom exactly vertical. The old 0.000001 mm
endpoint test classified a station on Baby Buzzard's slightly tilted nose edge
as interior, retaining a closed end. A tilted end also supplied a corner rather
than an end midpoint for Top/Side alignment and collapsed the first loft section.

## Decision
Register end edges in a temporary sampled outline, never in editable project data.
An explicit straight edge at the longitudinal extremum qualifies when its tilt
is at most 2 degrees and its physical axial drift is at most 0.5 mm. Move its two
sampled endpoints onto the original extreme plane. Preserve the original
nose-to-tail extent and adjacent sampled curves. Use the registered outline for
loft guides and the shared Side View transform, giving a complete end section
and its transverse midpoint at the registered plane.

Only the foremost/rearmost station within 0.5 mm of a qualifying end is registered
to that plane for loft interpolation and open-end detection. More interior
stations retain their positions. Curved, pointed and deliberately sloping ends
retain their geometry and the previous strict endpoint test. Scale the tests in
physical millimetres, including manually calibrated projects.

## Alternatives Considered
A larger station tolerance alone leaves the collapsed section/corner alignment.
Changing saved sketch points would alter the user's tracing. Flattening any
nearby sampled points could reshape curved noses and tails unintentionally.

## Consequences
Normal tracing discrepancies no longer imply a closed motor opening. The 0.5 mm
near-end band treats the outermost profile as an intended end profile; deliberately
closed ends require a profile farther inward. No persistent format change.

## Validation
fuselage_end_tests checks nose/tail registration, physical scale, angle/distance
limits, unchanged input sketches/extents, corrected alignment midpoint, inward
stations and preservation of pointed/curved ends. The read-only BabyBuzzard.foam
check confirms both nose edges qualify and its foremost station opens the nose.
Full component generation was not run under the current repository restriction.
