# ADR-0011: Spar grooves and mid-plane wing split
Status: Superseded
Date: 2026-09-14

Superseded by ADR-0012 for per-panel scope, Mid height, split geometry and file format.

## Context
Spars need independent top, bottom and mid-height options, percentage placement,
physical sizes, generated grooves/holes and alignment for gluing a split wing.

## Decision
Store three spar records in top/bottom/mid order. Size is diameter for round
spars or chordwise width for strips; strip height is inward groove depth. UI
uses project units, while state and files use millimetres. Local chord percentage
is measured from LE toward TE at each span location. Length runs from the root
to the given percentage of the half-span. Thus spar paths follow sweep/taper.

Cut the fixed wing after separating/beveling control surfaces, before mirroring.
Control bodies remain separate and are not split by spars. Sample 33 sections
of the actual fixed-wing skin, including tip treatment, to form smooth cutting
tools with maximum degree 3 and 1e-7 mm construction tolerance. Collinear
centers within 1e-7 mm are reduced to avoid redundant straight sections. Round surface grooves have their center at the top/bottom skin (nominal
semicircular depth); strip floors are the entered depth inward from that skin.
Mid spars use circular sections centered on Z=0, the existing common chord plane.
Split the fixed wing into upper and lower solids when Mid is enabled.

For each half-wing, add four cylindrical tabs to the lower solid, with matching
holes in the upper solid, at 20% and 80% of full half-span on both chordwise sides
of the mid-spar path. Tab diameter is 3 mm, engagement 2 mm, with 3 mm edge spacing
from the spar. Holes have 0.1 mm radial and depth allowance. Alignment locations
use the full half-span even when the spar is shorter. Reject insufficient local
material or disconnected results with a descriptive error; never omit a tab.

Write project version 3 and continue reading versions 1 and 2 with spars disabled.
Earlier applications reject version 3, avoiding silent loss of spar settings.
All lifecycle functions use the same state, unit conversion and fingerprint path.

## Alternatives Considered
A straight fixed-X spar would ignore the requested percentage on tapered wings.
Cutting the entire wing before control separation would split control surfaces
and violate the current single-fixed-body control-cut contract. Extra independent
alignment bodies would not locate the glued wing halves without assembly work.
Keeping version 2 would let older writers silently discard new spar settings.

## Consequences
There are two fixed solids per wing half when Mid is enabled, plus any controls.
Grooves follow sampled surface heights; they are not a guarantee that a rigid
straight stock spar fits a swept/curved path. No twist/dihedral is modeled yet.
Very thin wing tips, LE/TE positions and conflicting grooves may be invalid;
the app reports these rather than showing stale or incomplete geometry.
No new dependency is required.

## Validation
SparTests checks groove depth/width, round and strip shapes at top/bottom,
percentage length, mid-hole radius, split-body count, tab/hole engagement and
fit allowance, removed volume, tapered placement, mirroring and invalid sizing.
Panel and project tests cover independent options, units, persistence, legacy
files, invalid records and lifecycle reset. Full workflow smoke validation covers
panel visibility, regeneration and camera retention.

Alignment clearance uses vertical ray/material intervals at 16 perimeter samples
per tab to ensure a continuous column from 0.5 mm below the split through the
hole plus 0.2 mm skin allowance. Tabs and holes are grouped into one fuse and
one cut. Exact body validity is checked after the operations; the clearance
sampling remains a finite-resolution placement check.
