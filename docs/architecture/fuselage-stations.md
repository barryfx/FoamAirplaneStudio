# Fuselage profile stations

Profile Stations is enabled after both fuselage outlines are closed. Selecting it
checks its toolbar action, opens 2D and displays instructions at the top of the data
panel. This step places sections on Side View only; Top View remains visible and
locked. One station or an existing profile enables Edit Profiles; see fuselage-profiles.md.

Hover within eight screen pixels of the Side View upper or lower edge. A green
floating point and a vertical line preview show the complete proposed section.
Left-click once to place it between the edges, then move to place another. Both
anchors use the same scene X; screen Y determines top and bottom. Clicking empty
space or the Top View outline does not start a station.

The reusable ConstrainedLineEditor has an opt-in vertical, single-click placement
mode. It intersects all sampled curves in the active layer at the hovered X,
merges shared vertex hits and requires exactly two distinct boundary positions.
This supports separate edges and one periodic spline. Zero-height pointed ends
and ambiguous sections with more than two intersections cannot be placed. The
preview is omitted there. Source curves and fitting samples remain authoritative;
solid generation and nose registration are handled by FuselageSolidBuilder (fuselage-profiles.md).

Click a station body to select it (orange); Delete removes it. Click an endpoint,
move along its original outline curve, then left-click or Escape to drop it. Both
ends remain vertical. Invalid or duplicate moves retain the last valid position.
Escape clears selection. Leaving mode cancels an unfinished move and locks the
stations. All committed stations remain visible in other modes. Outline point edits
resynchronize attached sections; removing an attached curve removes its stations.

Version 10 stores fuselageStations independently from Wing stations, using upper
and lower curve anchors and transient selection saved as view state. Versions 1-9
start without profile stations. Hover/preview does not dirty the document; station
placement, committed moves and deletion do. New/Close clears stations; Save/Open
preserves them. Reference scale changes remap them through their source curves.
See ADR-0020 and formats/foam-project.md.

Deleting a station leaves its profile available for recovery. In Edit Profiles,
single-click an unattached profile to select/delete it, or select an unassigned
station and double-click the profile to reattach it. Both edits support Undo/Redo.
