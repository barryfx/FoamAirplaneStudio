# ADR-0017: Conservative wing lightening with split access
Status: Accepted
Date: 2026-09-15

## Context
The user requested hollow main-wing bays, minimum walls, uniformly spaced
crossmembers by count, a first-root setback and a setback before the last station.
Controls and tips must stay solid. Access requires a split even without a Mid
spar. Alignment pins and matching holes must retain the requested wall clearance.

## Decision
Store one whole-wing lightening configuration in backward-readable version 8.
Use unfolded span distances and global uniform rib centers; retain panel cap webs
in addition to requested ribs. Extend the existing main-only mid-height splitter,
using half thickness at 30% chord when no Mid spar exists. Hollow after structural
cuts and before dihedral/mirroring. Protect full-height columns around sockets
and pins, using socket radius plus wall thickness.

Construct conservative stepped prismatic pockets from clipped boundary triangle
envelopes (architecture/lightening.md). Include a mesh allowance in all three
axes. Only accept pockets crossing the complete local split-height interval.
Validate final halves as connected OCCT solids. Source sketches remain unchanged.

## Alternatives Considered
A direct whole-solid inward offset failed on the real GentleLady wing with a
2 mm wall; thin edges and intersecting small features make this unsuitable as
our required construction path. A fully fitted offset surface with arbitrary
self-intersection repair would add substantial geometric complexity. Voxel-solid
output would replace clean OCCT boolean solids with a much larger faceted model.

## Consequences
Pockets may leave more than minimum wall thickness, and have stepped floors and
ceilings. Very thin regions remain solid. The outer geometry, controls, spar
sizes and pin fit remain unchanged. The split is mandatory when enabled. There
is no new external dependency. The minimum-wall parameter is geometric and does
not certify structural strength or machining tool reach/corner radii.

## Validation
Focused tests cover minimum skin/end material, rib retention, mandatory splitting,
Mid/surface spars, pin support, untouched controls, mixed units and migration.
Spanwise intersections reject extra construction-slice partitions.
Project lifecycle tests cover saved/restored Lightening data and New reset.
Record Debug build, complete test outcomes and actual-project smoke results in
docs/baseline/lightening-validation.md.
