# Fuselage Formers

Assembly's Cut Intersections additionally cuts wing seats in the finished formers
using the wing's current placement and rotation. Formers remain separate parts;
stabilizers do not cut them. Undo Cuts restores their original geometry. Export
uses the cut geometry and retains the former's local section plane for DXF.

Formers replaces the former Firewall toolbar placeholder. It becomes available
when every Fuselage station has a closed profile and opens Side View in 2D.
The panel provides instructions, Width (thickness), Add Former, Rotation Angle and Delete Former.
Reference units apply to bare decimals; explicit mm/in suffixes override them. Each former and the next-former thickness retain their entered display unit through selection, undo and Save/Open.
The initial thickness is 3 mm and is valid without editing. Zero thickness is
rejected explicitly; positive thickness has no arbitrary minimum entry value. Selected formers show their own width; changing it
resizes about the center and also sets the next added former's thickness.
Changing Reference Wingspan preserves every existing former's physical thickness,
including mixed thicknesses. Its drawing width adjusts about its center, while
position and height continue to follow project scaling. Save/Open and undo restore
the adjusted rectangles at their saved scale without applying the change twice.
An incomplete Wingspan entry leaves the last valid thickness calibration intact.
Existing projects retain their current effective thickness on opening; this cannot
recover an older intended thickness already lost through earlier scaling.

Add Former creates a vertical rectangle with a margin above and below the Side
View bounds. It chooses the available fore/aft position closest to the view center,
allowing touching edges. If none fits, it reports that no position is available.
Drag inside to move in both axes, including up/down for a partial-height former.
Drag the top or bottom edge/handle to resize along the former's local height;
thickness stays fixed.
A rectangle can cross only the upper or lower Side View outline edge. The
former still fits the actual inner cavity across its width, but ends at the
opposite rectangle edge inside the fuselage. It need not cross both outline
edges; crossing both gives a full-height former. This uses the existing cavity
intersection and mask clipping, including for rotated rectangles.
Click empty space/Escape to deselect; Delete removes the selected former.
Completed rectangles remain visible across 2D modes and remap with references.

Positive-area Side View rotated-mask overlap is forbidden between formers and with
the servo tray. Add, angle/thickness changes, movement and resizing enforce the rule;
rejected edits retain the last valid placement and show an explanation. Tray
movement/resizing/creation is checked against formers too. Touching is permitted.
This conservative 2D rule also rejects mask overlap outside the actual cavity.
Save/Open and the model builder independently validate collisions.

Generation automatically enables hollowing when formers are present. Missing
station wall values use the existing 8 mm at/before LE and 5 mm aft defaults;
explicit values are preserved. Opening Thicken first is unnecessary. It uses the same Side View scale, X registration and
vertical origin as the tray and loft. Intersect each full-width X/Z slab with the
actual inner cavity, then subtract the supported fuselage body to remove any
ledge material. This creates fitted solids inside the skin without wall penetration.
A former wholly outside the cavity or resulting in disconnected solids produces
an error identifying its number. Partial height clips the section vertically.
Each former is retained separately, appended with the tray after wall Cut processing;
Cut splits the fuselage/supports, not these removable inserts. Before cuts, each former adds integral retaining rails immediately before and
behind its X extent, on both inner sides. Each rail is 4 mm wide along X and
projects 3 mm inward along Y. Cavity clipping follows the available full side
height even for partial-height formers; other former/tray solids are subtracted
so rails cannot penetrate removable inserts. Existing supports may merge with
rails. Empty portions beyond the cavity are omitted; disconnected supports
produce an error. The completed right body is reflected into a separate left part (fuselage-cuts.md). Assembly Export defaults formers to separate named solids in the combined STEP file, with individual DXF/SVG/STL files also available (export.md).

Version 15 persists rectangle masks and next-former thickness in physical mm.
Earlier versions load no formers; the legacy Firewall tool name maps to Formers.
Selection is transient. Model fingerprints include completed rectangles and their rotation angles,
so a next-thickness-only change does not regenerate. Generation uses the existing
independent Fuselage worker and reports former progress/count without rebuilding Wing.

Retainer construction skips insert cuts only when conservative bounding boxes
are disjoint. Potentially touching/overlapping inserts still use exact OCCT cuts.
The sideways-shifted clearance slice extends 0.1 mm beyond each end of the rail
in its local thickness direction. This avoids coincident end faces in the
subtraction, which can otherwise retain a cavity plug despite valid topology.
The rail itself remains 4 mm wide with 3 mm inward depth. An interior-centroid
classification check rejects residual solids whose interior centre remains
inside the clearance tool; it supplements, rather than replaces, final topology
validation. The padding rotates with the former and does not enlarge the rail.
If the cut fails, has invalid topology or retains a cavity plug, it is retried
with bounded fuzzy tolerances of 0.00001 mm and then at most 0.0001 mm, rather
than the normal 0.0000001 mm. All attempts preserve input shapes and use
cancellable OCCT progress. Each retry
must pass the same interior-clearance and topology checks; failure still stops
generation. It does not change the 4 by 3 mm rail dimensions. This addresses
BabyBuzzard36 former 4's front- and rear-right cuts returning the entire valid pocket despite
OCCT reporting success. See `../baseline/babybuzzard36-rail-fix.md`.
The compact error identifies the former in nose-to-tail order and front/rear,
left/right rail, without measurements. Parallel progress explains that the
operation builds support rails and clears their hollow centers.
See `../baseline/former-rail-clearance-fix.md` for the GentleLady diagnosis.
All completed rail shapes are fused to the shell in one multi-tool operation,
including overlap resolution for rails belonging to nearby formers. Progress
identifies the current former and the final wall-joining stage. Dimensions,
sampling, Boolean tolerance and final validity checks are unchanged.

## Per-former rotation

Rotation Angle appears directly below Add Former. It is a decimal degree entry
(three decimal places, -360 through 360), disabled without a selected former.
Each newly added former starts at 0. Negative degrees rotate counter-clockwise
in Side View about that mask's center; positive degrees rotate clockwise. Width
is measured normal to the former, and top/bottom handles resize along its local
height direction. Selection, overlay drawing, dragging, resizing, thickness
changes and tray/former overlap checks all use the rotated mask. Touching remains
allowed; edits causing positive-area overlap retain the last valid angle/position.

Generation rotates the full-width slab about model +Y through its mask center
before fitting it to the cavity and supported body. Retaining slabs rotate about
the same center, retaining 4 mm width normal to the former and 3 mm inward depth.
They extend through the cavity's full local height and clear removable inserts.
Both Fuselage and Assembly snapshots include angles; changing an angle invalidates
the shared Fuselage cache and dependent Assembly cuts. Project format 24 persists
one angle per rectangle; earlier projects initialize all angles to zero.

With mirrored fuselage construction, each former still fits the whole cavity and
remains one removable part. Only positive-Y retaining material is fused to the
right body; reflection creates its left counterpart. If a curved-roof clearance
band crosses the centre plane, both bands' positive-Y material is retained before
reflection. The complete final body contains separate main halves (ADR-0045).
