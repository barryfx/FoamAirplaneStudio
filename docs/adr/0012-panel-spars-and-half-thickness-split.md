# ADR-0012: Panel spar settings and half-thickness split
Status: Accepted
Date: 2026-09-14

## Context
Spars must be configured independently for each outline panel. A chord-plane
split fails on flat-bottom cambered profiles. The requested Mid location is
50% of local thickness at the chosen chord percentage. Ailerons and flaps must
remain intact.

## Decision
Use numbered spar tabs matching outline panel count. Each owns Top/Bottom/Mid
options and dimensions. Length is a percentage of the selected panel's span,
starting at its root; chord percentage still follows local LE to TE. Added
panels start disabled; removing an outline panel removes its spar settings.
Tab selection is saved view state and never invalidates geometry or dirties data.

When spars are present, partition only the fixed wing into panel span regions.
Internal boundaries use the projected span station of the next panel's root
chord midpoint in the existing root-derived wing frame. Each region receives
its own grooves. The existing control bodies bypass this partition and all
height splits, then are added back unchanged before mirroring.

Sample Mid height as (upper skin + lower skin)/2 at its specified chord fraction,
using the original uncut main panel over the physical spar length. The split is
level across the chord at each span station. Beyond the spar end it continues
at the terminal height to the panel tip, avoiding a joint that bends around tip
shaping after the spar has stopped. Mid hole and split share ruled span intervals.

Build a chordwise cutting sheet and split the main panel with OCCT Splitter.
Extend the sheet beyond the panel bounds. Require two valid solids and verify
volume conservation with adaptive integration before cutting the grooves.
Identify the upper solid with a skin-interior probe. Cut Mid first, then surface
grooves in both halves, using non-destructive Booleans to preserve shared inputs.
Tests exposed incomplete cuts with the reverse groove order on a flat-bottom
fixture. Top/Bottom retain their original surface-relative positions. Controls
are never included in this operation.

Alignment stays at 20%/80% of the panel span on both sides of Mid. Lower tabs
and upper holes follow the moved split. The cylinder covers the local split
height range across its footprint, retaining 2 mm minimum engagement and the
existing 0.1 mm radial/depth fit allowance. Material checks use the moved limits.

Project version 4 stores one three-spar array per outline panel and the selected
spar tab. Versions 1/2 initialize all disabled. Version 3 imports former global
settings into Panel 1 and initializes other panels disabled. No input file is
rewritten until an explicit save. This preserves existing dimensions/settings;
on old multi-panel files, review the new panel-relative length and configure
outboard tabs explicitly. The half-thickness rule also applies to migrated files.

## Alternatives Considered
Moving the whole wing or normalizing airfoil coordinates changes the intended
shape. A fixed global Z split cannot follow varying camber/thickness. Splitting
the full compound would split ailerons/flaps contrary to the explicit request.
Copying legacy global settings to every panel would silently add new features.

## Consequences
The main wing can contain independently split or unsplit panel bodies. Controls
can cross panel boundaries without being divided. Existing span-section sampling
and the root-derived coordinate convention still apply; this does not add twist,
dihedral, or an independently editable split surface. Finite sampling remains
an approximation between sections. No new dependency is required.

## Validation
Panel tests cover independent values, count changes, tab state and units. Codec
and lifecycle tests cover version 4, legacy migration and panel-count checks.
Geometry tests cover shifted and varying half-thickness splits, independent
panel locations and intact controls across panel joins. The original
Ailerons_flaps.foam is reproduced without modifying it.

ADR-0013 supersedes the whole-wing loft and cross-panel control ownership portions: panels now generate from their own stations and control settings. Other geometry rules remain in effect.
