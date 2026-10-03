# Fuselage Holes

Holes is available with the other fuselage tools once all profiles are complete.
It opens 2D and offers Top, Bottom, Left and Right wall choices. Top/Bottom use
the Top View outline; Left/Right use Side View. Model Left is negative Y.

Use Add Hole, Line/Spline, the hole selector and Delete Hole. Each connected
path is independently selectable and removable. Close every loop at its starting
point; Escape finishes a spline. Drawing and moving completed geometry outside
the outline is rejected. Leaving Holes with an incomplete loop shows a dialog
and retains the draft for correction. Generation independently rejects unclosed,
self-intersecting or out-of-outline holes.

The exact projected loop is extruded through the model, and the inner cavity
separates this prism into near/far cutter solids. Only the selected outside
component is subtracted. If it also reaches the opposite side, generation asks
for the hole to be moved/resized rather than cutting both walls. Therefore the
whole footprint must reach the cavity, not merely fit the exterior outline.
Variable thickness and asymmetric cross-sections do not require a fixed depth.
The isolation error identifies the wall and the one-based hole number in that
wall's path list. Changing Wingspan scales the drawn hole and exterior outline,
but retains physical wall thickness. A formerly valid hole can therefore extend
beyond the inner cavity after shrinking the model; move it inward or reduce its
size in Holes. Increasing CAD tolerances does not resolve a real clearance gap.

When isolation fails, an independent check traces sample paths from the actual
CAD hole boundary along the cut direction. If a path crosses fuselage material
but never reaches the inner cavity, generation reports that the named hole
overlaps a wall parallel to the cut direction. Top/Bottom holes can interfere
with side/end walls; Left/Right holes can interfere with roof/floor/end walls.
This diagnostic checks 33 parameter samples per boundary edge and honors Cancel.
Sampling is used only to confirm interference, never to approve a failed cut.
If it cannot confirm interference, the error retains the possibility of a CAD
isolation failure instead of asserting that the drawing is wrong.

Holes are applied to the supported, reflected halves after rails and before Cut
and alignment. They stop at the original inner cavity and do not remove
internal supports or separate former/tray inserts. Alignment avoids the resulting
openings. Cut retains detached bodies: boundary-crossing paths split both walls,
while closed interior paths split only the selected wall (fuselage-cuts.md).

Project format 25 saves four fuselageHoles layers, tools and pending drafts.
Older files load empty holes. Hole geometry invalidates Fuselage and Assembly
and follows reference remapping. Open stays in 2D without automatic generation.
See ADR-0038.
