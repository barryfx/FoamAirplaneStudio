# ADR-0052: Mirrored fuselage stiffeners
Status: Accepted
Date: 2026-10-03

## Context
Carbon strips or round rods need matching grooves down the fuselage boom. The
user specified a per-side count mirrored on both sides, and one diameter for
round stock, regardless of rod or tube construction.

## Decision
Persist one shared stiffener specification in additive project format 33; older
projects default to disabled. Reuse spar shape types and carbon material records,
with a body-frame flag to preserve fuselage centroids during wing placement.
Route equal height fractions along the outer wall, form smooth lofted tools,
and cut before surface holes/cuts. Round grooves embed half the diameter, as
wing surface spars do. Weight assumes solid carbon stock because no bore is
specified. Actual wall checks and a sampled bend guard reject unsuitable routes.

## Alternatives Considered
Independent paths per stiffener would require a new route editor and more saved
inputs. Straight grooves would leave curved booms. Full-depth round bores would
prevent surface insertion and differ from wing spars. Neither is used here.

## Consequences
Settings are compact and unit-independent. Count, size or range may need adjustment
at narrow or sharply changing sections. The bend guard is not a material-specific
minimum bend radius. No new dependencies or incompatible legacy migration are
introduced. Carbon stocks affect weight, not export component enumeration.

## Validation
Dedicated tests cover mirrored counts, strip/round volume, carbon centroids,
placement, wall rejection, unit entry, format migration and full hollow-fuselage
generation. Build and suite results are recorded in the stiffener validation log.
