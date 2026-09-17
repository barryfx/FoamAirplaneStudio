# Fuselage Formers

Formers replaces the former Firewall toolbar placeholder. It becomes available
when every Fuselage station has a closed profile and opens Side View in 2D.
The panel provides instructions, Width (thickness), Add Former and Delete Former.
Reference units apply to bare decimals; explicit mm/in suffixes override them.
The initial thickness is 3 mm and is valid without editing. Zero thickness is
rejected explicitly; positive thickness has no arbitrary minimum entry value. Selected formers show their own width; changing it
resizes about the center and also sets the next added former's thickness.

Add Former creates a vertical rectangle with a margin above and below the Side
View bounds. It chooses the available fore/aft position closest to the view center,
allowing touching edges. If none fits, it reports that no position is available.
Drag inside to move in both axes, including up/down for a partial-height former.
Drag the top or bottom edge/handle to resize vertically; thickness stays fixed.
Click empty space/Escape to deselect; Delete removes the selected former.
Completed rectangles remain visible across 2D modes and remap with references.

Positive-area Side View rectangle overlap is forbidden between formers and with
the servo tray. Add, thickness changes, movement and resizing enforce the rule;
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
Cut splits the fuselage/supports, not these removable inserts. Formers do not add
wall slots or ledges. Export controls remain future work.

Version 15 persists rectangle masks and next-former thickness in physical mm.
Earlier versions load no formers; the legacy Firewall tool name maps to Formers.
Selection is transient. Model fingerprints include only completed rectangles,
so a next-thickness-only change does not regenerate. Generation uses the existing
independent Fuselage worker and reports former progress/count without rebuilding Wing.
