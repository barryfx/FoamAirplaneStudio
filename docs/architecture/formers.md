# Fuselage Formers

Formers replaces the former Firewall toolbar placeholder. It becomes available
when every Fuselage station has a closed profile and opens Side View in 2D.
The panel provides instructions, Width (thickness), Add Former, Rotation Angle and Delete Former.
Reference units apply to bare decimals; explicit mm/in suffixes override them.
The initial thickness is 3 mm and is valid without editing. Zero thickness is
rejected explicitly; positive thickness has no arbitrary minimum entry value. Selected formers show their own width; changing it
resizes about the center and also sets the next added former's thickness.

Add Former creates a vertical rectangle with a margin above and below the Side
View bounds. It chooses the available fore/aft position closest to the view center,
allowing touching edges. If none fits, it reports that no position is available.
Drag inside to move in both axes, including up/down for a partial-height former.
Drag the top or bottom edge/handle to resize along the former's local height;
thickness stays fixed.
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
produce an error. The main body is then split left/right (fuselage-cuts.md). Assembly Export defaults formers to separate named solids in the combined STEP file, with individual DXF/STL files also available (export.md).

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
