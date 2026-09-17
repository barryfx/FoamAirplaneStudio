# Fuselage Servo Tray

Servo Tray opens the Side View placement workflow in 2D once the Fuselage profiles
are complete. Instructions precede Width and Height fields, Place Rectangle and
Delete Tray. Width is fore/aft length; Height is tray thickness. Fields use Reference
units and accept explicit mm/in suffixes. Defaults for a new tray are 100 mm by
3 mm. Editing a dimension or clicking Place Rectangle creates a rectangle centered
on Side View. Drag inside or at an edge to move it with fixed dimensions. Change
fields to resize about the existing center. Delete removes it; Escape deselects.
There is no freehand drawing or corner resizing. Invalid input restores the previous
value. A completed rectangle remains visible in other 2D modes and remaps with the
reference. Physical dimensions convert through the same Side View scale as generation.

A placed tray automatically enables hollowing on regeneration. Missing station
walls use their existing LE-based defaults; explicit values are preserved. The
default tray thickness is valid without editing. Zero thickness is rejected. The builder retains
that same validated cavity shape from the wall-generation step. A horizontal
slab spanning the rectangle's length, height and full cavity width is intersected
with the cavity. The resulting tray is a separate positive-volume solid, fitted
to the actual inside surfaces without wall penetration. Side edges therefore
follow the cavity through the tray thickness; on sloping walls the top and bottom
edge contours can differ. The horizontal top face(s) and their boundary wires
are retained separately as the source for future 2D laser export. This change
does not introduce a DXF/SVG export action or a bevel/clearance allowance.

The support region is a 5 mm high cavity slice immediately below the tray's
underside, along the tray length. Subtract copies translated by +5 and -5 mm in
Y to obtain the two bands extending 5 mm inward from the respective inner sides.
Fuse these ledges into the fuselage wall, retaining the removable tray separately.
This applies the user's revised 5 mm inward dimension (superseding 3 mm).
Missing cavity, outside placements, disconnected tray/support geometry and
insufficient 5 mm support height report descriptive errors. No automatic relocation
or wall-thickness reduction is performed.

Pipeline: exterior -> Thicken -> support ledges -> Fuselage Cut -> separate tray
assembly -> mesh. Thus hatch cuts split the walls and ledges together, while the
tray stays an independent part. The model result retains the supported/cut body,
tray, assembled display shape, and tray top faces. The existing Fuselage worker,
cancellation checks, stale-result protection and component cache apply. The GUI
retains top faces for later export and reports tray/support completion. Wing inputs
and its cached model are unaffected.

Version 14 stores `servoTray.rectangle` in scene coordinates. Width and Height
are derived from that rectangle and the Reference scale; no format change is
needed. Legacy `first` and `drawing` fields remain readable, but obsolete unfinished
freehand drafts are cleared on restoration. Newly saved trays use null/false for
these fields. The Fuselage model fingerprint includes the completed rectangle.
New/Close clears both the saved rectangle and generated tray outline cache.
