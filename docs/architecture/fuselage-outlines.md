# Fuselage outline editing

Fuselage is available after Reference and Wing airfoil assignments are complete.
Entering it selects the checked Outline action and the 2D viewport. Outline shows
instructions above Top View and Side View buttons. Both initially are off. Clicking
one lights it, disables the other, and reveals its Line/Spline row directly below
it. Click the active view again to release it and enable the other view.

The existing SketchEditor supplies the same snapping, fitted curves, point dragging,
curve selection/deletion and Escape behavior as Wing. Fuselage has a separate editor
with exactly two layers: Top (0) and Side (1). Only the selected view is editable;
both outlines remain visible across workspaces without changing Wing or airfoils.
Line finishes after two points; Escape finishes a pending spline; clicking the first
point after three distinct spline points closes it. Turning a tool or view off
finishes a valid pending curve. Tools remain selected after drawing. Leaving the
view releases selection and dragging. Zoom and scroll use the existing viewport.

Leaving Fuselage Outline through a component/workspace action or switching to 3D
finishes pending geometry and checks both layers. A warning lists Top View, Side
View, or both if they are empty, open, branched, disconnected, or have zero sampled
area. The warning permits leaving and never discards the sketches. Merely releasing
Top/Side does not warn about the other unfinished view. Project replacement/reset
restores snapshots without spurious outline warnings; unsaved-data prompts remain.

Both valid outlines enable Profile Stations immediately, while Outline stays
selected. Clicking Profile Stations opens single-click vertical Side View section
placement; see fuselage-stations.md.
Deleting or opening a boundary locks Profile Stations again. One station enables Edit Profiles; closed profiles at every station unlock the remaining actions. Closure checks use the same sampled-boundary helper as airfoil
traces; they do not certify machining feasibility or create fuselage solids.

Version 9 introduced storage for both layers, pending geometry, active tool/view and selections.
Versions 1-8 open with two empty outlines. View/tool selection alone is not an
unsaved change; committed geometry and pending points are. Fuselage does not enter
the Wing generation fingerprint. Same-image reference scale changes remap both
layers alongside the other sketches. New/Close clears both layers. See ADR-0019
and the project format specification.
