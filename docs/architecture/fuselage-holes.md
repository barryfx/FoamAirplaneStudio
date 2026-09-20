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

Holes are applied to the supported body after rails and before Cut and centre
splitting/alignment. They stop at the original inner cavity and do not remove
internal supports or separate former/tray inserts. Alignment avoids the resulting
openings. Existing Cut still splits through both walls and retains cut-out bodies.

Project format 25 saves four fuselageHoles layers, tools and pending drafts.
Older files load empty holes. Hole geometry invalidates Fuselage and Assembly
and follows reference remapping. Open stays in 2D without automatic generation.
See ADR-0038.
