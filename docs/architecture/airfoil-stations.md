# Airfoil stations

Selecting Wing / Airfoil Stations shows the placement instructions and enters
the sketch editor's constrained-line mode. Outline editing and outline point
handles are disabled. All outlines remain visible in light blue. Stations are
magenta with white backing strokes and remain visible in other workspaces.

Hover within eight screen pixels of an outline curve to see a green moving point.
Click LE first, then TE to place a station; placement immediately re-arms for
another station. Only points on outline curves are accepted. The user identifies
which edges are LE/TE, within the selected panel. Within 15 degrees of
vertical or horizontal, the second point snaps to that axis through the first
point, provided it intersects the hovered TE curve. Otherwise the station remains
free. The chosen alignment is retained. Duplicate station lines are ignored.

Click a station body to select it in orange. Delete removes the selection;
Escape clears it. Endpoint hits take precedence over line-body selection:
click an endpoint, move the mouse to slide along its original curve, then left-click
again or press Escape to drop. The mouse button need not stay down. For axis-aligned stations,
the paired endpoint moves along its own curve to retain alignment. Moves beyond
the paired curve's available intersection are rejected. Both left-click and Escape
commit the latest valid position, even if the pointer is currently beyond the
available intersection. Escape also cancels incomplete placement. Leaving the mode cancels
unfinished interactions and locks station editing. Returning to Airfoil Stations
allows editing again.

Two committed stations on every panel unlock Airfoils, while Airfoil Stations remains selected.
This supplies the usual root/tip pair; additional stations can be placed in order.
Deleting stations re-evaluates the prerequisite. Airfoil assignment is provided
by Wing / Airfoils (see airfoils.md). Assigned profiles now generate the mirrored
wing in the 3D viewport (see wing-solids.md).

ConstrainedLineEditor owns the station records separately from outline geometry.
Read-only lines() exposes LE/TE attachments, coordinates and alignment to future
profile interpolation tools. Source-curve removal invalidates its stations;
source point moves update surviving attachments. See ADR-0003 for storage and
accuracy choices. Stations, attachments and assignments are now saved to `.foam`
project files; see projects.md and ADR-0006.

## Panel ownership
Numbered tabs match Outline. Both LE/TE clicks snap only to the selected panel.
Only that panel's stations can be selected, moved or deleted; all remain visible.
Two stations are required on every panel. Shared-boundary stations are distinct
records and can have different airfoil assignments. See ADR-0013.

Station placement checks the LE-to-TE direction against committed stations,
including other panels. A nonpositive direction dot product is inconsistent:
a popup explains the conflict, the new station is not committed, and placement
re-arms for a fresh LE-first pair. The existing station may be the reversed one;
it can be deleted and redrawn. Saved files are not silently changed. Generation
also validates directions and identifies conflicting station/panel numbers.
