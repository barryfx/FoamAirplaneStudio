# Sketch editing

Wing / Outline displays a number-of-panels spinner and tabs numbered 1–N. Each
tab contains the planform instruction and Line/Spline toggle buttons. Clicking an
active button turns it off; selecting the other tool switches tools.

Line commits after two distinct points. Spline interpolates multiple picked points
using OCCT and commits on Escape. A two-point spline is straight. Tools remain on
after completion, ready for another curve. Switching tools, tabs or workspaces
also commits a pending curve with at least two points; a lone point is discarded.
Duplicate interpolation nodes are ignored. Invalid interpolation is not committed.

Snapping uses eight viewport pixels at any zoom. A shared point within a panel is
stored once and referenced by all incident curves. Other panels can supply snap
coordinates but never share editable identities. With both tools off, left-drag
moves a point in the selected panel and refits its incident splines. Moves creating
duplicate curve nodes or failed fits are rejected. Every panel remains visible
across workspace changes; only Wing / Outline permits outline edits. All completed curves
use light blue with a dark blue border, including in Airfoil Stations and other
non-editing modes. The color and screen-space stroke width do not fade when
editing ends. Only the selected editable panel shows point handles. Pending curves use
orange. Wheel zoom and explicit scrollbar scrolling remain available.

With both drawing tools off, click a line or spline within eight screen pixels
to select it. The selected curve is drawn orange with a thick white halo for
contrast over reference artwork. At overlaps the last drawn curve is selected.
Only the active panel can be selected. Delete removes the selected curve and
only those points no remaining curve uses. Shared junctions remain intact.
Clicking empty space, Escape, beginning a point drag, changing panel count/tab,
activating a drawing tool or leaving Outline clears selection. Delete with no
selection, while drawing, or outside Outline does not alter sketches.
Completed outlines continue to render with editing inactive, including Reference
and other component workspaces; changing selection never hides any outlines.

SketchEditor is a reusable QObject/controller and layer model. PlanViewport owns
it and calls its foreground painter; WingOutlinePanel owns only wing controls.
New resets the editor. Removing panels with the spinner removes their outlines;
the spinner tooltip explains this behavior.

Airfoil Stations uses the editor's separate ConstrainedLineEditor collection;
outlines are locked during that mode. See airfoil-stations.md and ADR-0003.

Coordinates follow the reference scene: pixels for an uncalibrated reference,
millimeters for actual scale. Switching the same reference's scale mode remaps
points page by page to preserve alignment. Replacing the reference preserves
coordinates; it does not automatically register a different image. Aircraft
wingspan calibration is implemented by WingSolidBuilder; fuselage calibration is handled by FuselageSolidBuilder (fuselage-profiles.md). `.foam` projects preserve sketch layers, meanings, pending points
and selected tools (projects.md and ADR-0006).
Without a reference, the sketch canvas starts at 1000 by 700 scene units.

Outline readiness is a connectivity check: each inner panel has two unbranched
open chains; the last panel has one open chain with at least three distinct nodes,
representing leading edge, tip and trailing edge. This unlocks Airfoil Stations
without switching away during editing. It does not verify aerodynamics, edge
classification, intersections or manufacturing suitability. Adding an empty panel
disables downstream steps until its outline is defined.

The validator checks each connected series separately for two open endpoints;
panel completion is exposed by panelOutlineDefined. Whole-wing completion uses
all panel results, including panels whose tabs are not selected. Inner-panel
LE and TE are deliberately independent; no connection between them is required.

Idle sketch finalization and panel selection do not resynchronize station anchors. This avoids numerical drift from changing saved data during view-only navigation; actual outline edits still synchronize attachments.
