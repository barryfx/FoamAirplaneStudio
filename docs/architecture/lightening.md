# Wing lightening

Wing / Lightening contains a whole-wing Enable Lightening checkbox, initially off.
Its description explains access, retained material and the following fields:

- Wall Thickness: minimum material around the finished main-wing skins, LE/TE,
  control separation faces, spar grooves/holes and alignment features. Default 2 mm.
- Number of Crossmembers: 0-100, default 4, along each half-wing's hollowing range.
- Crossmember Thickness: spanwise solid rib width, default 3 mm.
- Start Distance from Root: unfolded distance from the first panel root, default 25 mm.
- Stop Distance before Last Station: inward distance from the last station on the
  outermost panel, default 25 mm. For an oblique station use its inboard endpoint.

Lengths accept mm/in suffixes regardless of project units and retain entered text.
Bare numbers use current units. Start/stop can be zero; thicknesses must be positive.
Maximum lengths are 10000 mm. Invalid edits restore the last committed value.
Unchecking retains settings. Old files initialize lightening disabled.

Place N crossmember centers at start + i*(end-start)/(N+1), i=1..N. Hollowing bays
stop at their half-widths. Reject an empty range or ribs too thick for this pitch.
Panel caps also retain at least wall thickness, so panel-joint webs are additional
to the requested crossmembers. Distances use generated unfolded panel lengths,
including near-terminal internal truncation; dihedral does not change spacing.
The rounded tip and everything beyond the final cutoff remain solid.

## Geometry and access

Apply lightening after separating controls and constructing spar cuts, before
panel dihedral placement and mirroring. Only the main body is affected. Ailerons
and flaps remain solid and are not split. Every main panel splits into upper/lower
bodies whenever Lightening is enabled, even without a Mid spar. With a Mid spar,
retain its existing half-thickness split and alignment pins. Otherwise the split
follows half thickness at 30% chord; no additional alignment pins are invented.

Cavities are conservative stepped pockets, not a uniform inward-offset shell.
Unify redundant coplanar loft faces without changing the geometry. Triangulate the finished main body at 0.01 mm linear / 0.08 rad angular deflection.
Clip boundary triangles against each candidate column expanded in X and Y by
wall + 0.02001 mm. Their clipped Z extrema, padded by the same amount, bound the
floor and ceiling. This axis-aligned envelope retains at least the requested
wall with mesh allowance, and may retain more at steep or thin surfaces. A ray
checks that the pocket center is within material. Skip columns blocked by a
surface at the split, insufficient thickness, or a cavity not spanning the full
local split height range. Thus pockets are accessible through the split and
narrow LE/TE regions stay solid rather than breaking an offset operation.

Use 24 chord columns and span slices no longer than max(25 mm, panel span/12),
separately inside each bay. Adjacent columns become one stepped prismatic tool;
the prism tools are fused and simplified before subtraction so no coincident
caps or extra ribs remain at column/slice boundaries. Continuous pockets
may have steps in their floors and ceilings. Regions unable to fit a pocket
remain solid. The actual BRep solid stays authoritative for the final cuts.

Before subtracting cavities, remove full-height support cylinders around all
Mid spar alignment features. Radius is the 1.6 mm matching-hole radius plus Wall
Thickness (larger than the 1.5 mm pin). These columns connect the alignment
features to both skins, preventing hollowing from leaving unsupported pin islands.
Original peg/socket fit and placement remain unchanged. Check both hollowed
halves for valid topology and one connected solid before attaching pins/cutting
sockets. Errors identify the owning panel and operation.

State edits use the existing dirty comparison and deferred regeneration rules.
Navigation does not rebuild; updates retain the camera. Processing uses the wait
cursor and status text. Save/Open/New/Close/Save As use the current format version 25 (Lightening was introduced in version 8).
See ADR-0017 for the algorithm choice and limitations.
