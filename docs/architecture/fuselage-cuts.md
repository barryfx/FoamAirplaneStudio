# Fuselage cuts

Cut becomes available when every station has a closed profile. Top, Bottom,
Left and Right View selectors choose the target surface. Top/Bottom use the Top
outline; Left/Right use the Side outline. Add Cut, the path selector, Line/Spline
and Delete Cut retain their existing editing behavior. Switching surfaces finishes
the current draft. All four layers remap with the reference image and support
undo/redo and project persistence.

A connected path that reaches or crosses its outline boundary creates a cutting
sheet through the entire body, cutting both opposite walls. A path wholly inside
the outline cuts only the selected wall: the inner cavity divides the sheet,
and only faces connected to the selected exterior starting plane are retained.
If the sheet cannot reach the cavity without reaching the opposite wall,
generation reports an error rather than cutting the wrong side. Before splitting,
the entire enclosed hatch footprint is extruded and divided by the cavity. If
the selected exterior portion still reaches the far side, a parallel wall
interferes with the hatch. This also detects a wall inside the loop that a
perimeter-only check misses. Generation stops and the status message identifies
the surface and cut number, asking the user to move or shrink the cut over the
inner cavity. Invalid CAD isolation also stops generation. These checks use
cancellable Boolean operations rather than sampled rays.

Interior paths require a hollow fuselage and a closed loop to detach a piece.
Open through-paths must reach both outline edges to separate a body. Branches,
self-intersections, disconnected wires and paths that separate no body report
the affected surface/path. Line edges remain analytic; splines use OCCT
interpolation. The operation splits solids; it never subtracts hatch material.
Both through-cuts and single-wall cuts retain every cut-out as a separate
component at its original placement, with no kerf or material loss. Each split
requires increased solid count, valid positive-volume solids and conservation
of volume within max(0.001 mm3, one part per million).

Format 31 stores Top/Bottom/Left/Right cut layers. Earlier two-view files map
Top to Top and Side to Left, preserving points, active view and pending draft.
Interior legacy paths now follow the selected-wall rule; boundary-crossing paths
remain through-cuts. Geometry fingerprints include all four layers. The
low-level two-layer geometry API remains available for legacy regression callers.

The two main fuselage halves stay separate. Detached hatch pieces crossing their
mating plane are joined into whole cut-out components; hatches belonging to one
wall remain independent. Formers and trays are appended whole after these cuts.
Cancellation, invalid-result checks and stale-result protection remain active.
See ADR-0049 for the four-surface decision.

## Half alignment
Complete model generation adds four mating alignment features to the retained
main halves: two at the top seam and two at the bottom seam. Pins on the negative-Y
half project 3 mm; same-diameter sockets on the positive-Y half are 3.5 mm deep.
Diameter is 4 mm or the local wall thickness if smaller. Wall values use the
same station interpolation as generation, additionally capped by available skin.
Targets are 15%/75% of the remaining main-body length; nearby positions within
5-35%/65-95% are searched to avoid hatches, cavities and thin ends. The uncut
body supplies the true skin location; exact containment in the post-cut main
body ensures support, including 0.1 mm beyond each blind socket. No extra radial
clearance is added. If all four cannot fit, a location-specific error is shown.
Pins and sockets are applied in two batched Boolean operations, with valid
single-solid halves and feature-volume checks. Cut-outs remain unchanged.
See ADR-0032. The plain split helper can omit features for isolated cut tests;
the complete Fuselage builder always requests them. Saved project inputs are
unchanged; alignment features are regenerated, not serialized.

The four pin-location searches now run independently with private CAD copies and
intersection state. Ray caches load only faces whose bounds meet each query;
oriented crossings supply material intervals without eager whole-solid
classifiers. Exact containment still certifies every pin. Candidate conflicts retry in the original
placement order. Pin fusion and socket subtraction then run concurrently on their
respective halves. Holes/cuts finish first because they determine available stock.
The serial processing option preserves a comparison path.
