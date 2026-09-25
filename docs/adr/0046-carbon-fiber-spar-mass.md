# ADR-0046: Carbon fiber spar material accounting
Status: Accepted
Date: 2026-09-24

## Context
Wing spars previously cut foam grooves but contributed no automatic mass.
Mid-height spars need independently entered outside and inside diameters.

## Decision
Treat all enabled wing spars as carbon/epoxy composite. Measure density-independent
volume and centroid during wing generation using the same sampled paths and
parallel section planes as the grooves. Surface rods use full circular sections;
strips use the finite entered width and inward depth. Mid tubes subtract bore
volume and moments from outer volume and moments, without a new boolean cut.
Use nominal root-to-percentage span endpoints, excluding boolean overrun at
mitered caps. This models nominal stock length, not custom angled end trimming.
Transform these records with dihedral, mirror and Assembly placement; retain
them through foam interface cuts. Keep them separate from foam geometry,
Inspect/export components and plywood inserts. Generation fingerprints include
ID; density editing reuses cached material measurements.

Format 29 stores ID in mm plus explicit display text, and CF density. Older
projects retain OD, default ID to max(0, OD minus 1 mm), and density to 1540 kg/m³.
A zero bore represents a solid rod. New Mid defaults are 6 mm OD and 5 mm ID.

1540 kg/m³ is a plausible starting estimate, not a stock-specific certification.
For comparison, [DragonPlate](https://dragonplate.com/how-to-spot-the-difference-between-good-quality-and-poor-quality-carbon-fiber)
lists 1.55 g/cm³ (1550 kg/m³), very close to the chosen default. [Rock West's carbon/epoxy sheet/bar datasheet](https://www.rockwestcomposites.com/on/demandware.static/Sites-RWC-Site/Sites-rwc-master-catalog/-/downloads/RWC_forgedblocks_tds.pdf)
lists 1.6 g/cm³ (1600 kg/m³). Construction differs from pultruded tubing; the
editable density should be replaced with the supplier's figure when available.

## Alternatives Considered
Manual added-part weights duplicate spar data and become stale after geometry
changes. Measuring groove tools overcounts strips and tube bores. Including
carbon solids in the foam compound misclassifies their mass and export material.

## Consequences
Automatic spar weights and mass centers follow the generated wing, including
both halves. Added-part entries for those same spars should be removed to avoid
double counting. Nominal stock follows the existing modeled groove path; this
is not a structural analysis or a fabrication allowance for angled end trimming.

## Validation
Targeted spar geometry/mass, panel input, persistence, Assembly transform, and
Weight and Balance GUI/cache checks; results recorded in the validation log.
