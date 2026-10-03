# ADR-0050: Fiberglass patches for Weight and Balance
Status: Accepted
Date: 2026-10-03

## Context
The user requested named, multi-shape fiberglass regions on all four component
workspaces, one-sided or wrapped coverage, and cloth plus resin mass in CG.
Open chains may end outside the outline. Resin quantity must use thickness,
defaulting from cloth weight; existing material densities move into a dialog.

## Decision
Reuse SketchEditor layers with separate patch metadata and a teal appearance.
Clip a loop against the drawn outline; close open chains only outside that
outline to avoid silently choosing which side of a crossing line to cover.
Project masks onto private meshes of current Assembly components and integrate
covered 3D triangles, including edge-on faces and mirrored/dihedral panels.
Reject hidden interior surfaces with ray visibility checks. Preserve separate
patch contributions so overlaps represent multiple layers. Keep covering out
of solid generation and exports; include it in mass, moments and wing loading.

Persist additive fields in format 32, retaining readers for versions 1–31.
Cache area/centroid independently of material values. Store resin-only equivalent
thickness per patch, with an automatic default and a manual override. Material
density edits are accepted atomically through a modal dialog; Cancel changes none.

Use 1180 kg/m³ resin density: the cured specific gravity of the solvent-free
[WEST SYSTEM 105/206 epoxy](https://www.westsystem.com/app/uploads/2022/12/105-206-Epoxy-Resin.pdf)
is 1.18. Its data sheet identifies foam and reinforcing fabrics among substrates.
This is a starting value for foam-compatible laminating epoxy, not a guarantee
about every foam/resin combination.

Assume glass density 2550 kg/m³ and bulk cloth density 900 kg/m³ for the automatic
thickness estimate. The bulk estimate gives approximately 0.151 mm for 4 oz/yd²,
within the published 0.005–0.008 inch range for
[Fibre Glast 4 oz woven cloth](https://www.fibreglast.com/products/4-oz-fabric-50-inch-wide-262).
Fill the estimated void volume and add 0.01 mm resin equivalent for a thin coat.
The selected packing density and coat allowance are engineering assumptions;
cloth weave, application and sanding can change actual resin uptake.

## Alternatives Considered
Projected 2D area times two misses curvature and sidewalls. Adding material to
CAD changes manufactured geometry and complicates all component builders.
A fixed resin/cloth mass ratio disregards the requested thickness and density
controls. Exact trimmed-surface integration would be more expensive and still
would not determine actual resin uptake or fabric conformability.

## Consequences
Surface estimates have mesh and fragment-visibility approximation error.
Overhangs, sharp seams and complex occlusion need judgment; results are mass
estimates, not structural analysis or guaranteed layup weights. Input geometry,
projection and material assumptions are documented in architecture/fiberglass.md.

## Validation
Analytical boxes, hollow shells, cylinders and circles exercise area and mass
without Wing/Fuselage generation. GUI checks cover drawing, units, metadata and
the material dialog. Placement/dihedral, persistence, migration and cache checks
are recorded in baseline/fiberglass-validation.md.
