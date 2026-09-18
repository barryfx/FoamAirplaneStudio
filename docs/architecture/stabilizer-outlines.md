# Stabilizer outline editing

Horiz Stab and Vert Stab have the same toolbar: Outline, Airfoil, Hinge Line, Cut.
Their existing eligibility after completed Fuselage profiles remains unchanged.
Entering either workspace selects Outline in 2D. Until a valid open outline with
nonzero enclosed area and an explicitly selected leading-edge endpoint is present, only Outline is enabled. Airfoil uses one imported
profile per stabilizer. Hinge Line separates and bevels the control surface (stabilizer-hinges.md). Cut removes material with closed Line/Spline shapes (stabilizer-cuts.md).

PlanViewport owns two independent single-layer SketchEditors. A shared
StabilizerOutlinePanel supplies instructions and Line/Spline toggles. Editing
matches Wing: two-point lines, multi-point fitted splines, Escape to finish,
endpoint snapping, point dragging, curve selection and Delete with both tools off.
Only the active component's Outline in 2D accepts input. Completed sketches remain
visible across 2D workspaces and reference scale changes remap both editors.

Trace one continuous open chain around the leading edge, tip and trailing edge,
leaving the root open. Horizontal instructions request the right half including
elevator, describe mirrored generation and later rudder-clearance cuts. Vertical
instructions request the complete fin including rudder, without mirroring.
3D generation now creates mirrored horizontal halves or one vertical fin; see
stabilizer-solids.md.

On leaving Outline or entering 3D, finish pending geometry and check the chain.
Nonempty disconnected, branched or closed sketches produce a warning. For one
open chain, check the line through its two endpoints, independent of curve order
or direction. Accept angles within 10 degrees (inclusive) of horizontal or vertical,
so a drawing rotated 90 degrees passes. The test depends only on the endpoints,
not the interior curve shape, zoom or uniform scale. Coincident endpoints warn
because they cannot define a line. Warnings allow navigation and preserve geometry;
empty sketches do not warn.
These checks describe drawing intent, not manufacturing validity.

Project version 18 retains both sketch states (introduced in version 16), including unfinished work, tools and
selections. Restoration suppresses warnings and reinstates drafts after mode
activation. New/Close resets both. Geometry and pending points affect dirty state;
idle tool/navigation changes do not. Neither sketch enters Wing/Fuselage model
fingerprints. See formats/foam-project.md and ADR-0027.

The Select Leading Edge End Point button finishes the drawing tool and enters a
one-click selection mode. Only open endpoints can be selected; interior points
and empty space leave selection mode active. Escape cancels. The chosen endpoint
has a green LE marker and is required by both toolbar readiness and generation.
It follows point dragging and scale changes; deletion remaps its point index or
clears it if removed. Extending that endpoint clears the choice because it is no
longer an open endpoint. The selection affects dirty state and geometry caches;
the temporary picking mode does not. Version 18 persists the chosen point index;
older files require the user to choose it before generation.

Hinge Line drawing and relief are described in stabilizer-hinges.md.
