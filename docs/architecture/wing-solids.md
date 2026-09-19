# Wing tips and automatic 3D generation

Wing / Dihedral shows numbered panel tabs and a Root Dihedral field in degrees.
Each value is an increment relative to the preceding panel; the first panel is
relative to horizontal. Positive values raise the tip. For example 3 then 5
produces absolute panel angles of 3 and 8 degrees. Defaults are zero. Tab changes
are view state, while angle changes invalidate the model without resetting its
camera. See dihedral.md and ADR-0016.

The outermost wing tip always has a rounded profile looking along the chord.
Its upper/lower skins converge toward their midpoint using a circular retention
profile with radius half the local thickness, preserving the traced planform.
Additional cosine-spaced sections resolve the rounding near the outer end.
Tiny closure thicknesses maintain valid OCCT topology. No tip-type choice is
exposed or stored in new projects; legacy tip choices are ignored on migration.

Once every station has an airfoil and the outline/stations are complete, any
Wing tool can display the mirrored solid in 3D. Zero-degree dihedral is valid even
without visiting its panel. Edits while viewing 2D defer generation until entering
3D; edits while viewing 3D trigger regeneration. Toolbar changes retain the chosen
viewport and do not rebuild unchanged inputs. Subsequent model updates retain
the full camera state; only the first display fits the new wing automatically.
Incomplete or invalid geometry clears the old model and shows a status
message. The source outline and stations remain editable in their own 2D modes.

The half-wing is mirrored at the root section joining the first panel's two open
root endpoints, even if they are offset in the image or the first station is
angled or placed outboard. Temporary panel closure edges support section
intersections without changing the sketch. Full manual Wingspan
calibrates the traced span, with uniform chord/thickness scaling. Actual-scale
reference coordinates are already millimetres. Profiles interpolate between
stations within each panel and hold the nearest profile beyond their range. Trace airfoils with LE
left and TE right. See ADR-0005 for section sampling, coordinates, tip construction
and current limits; ADR-0007 updates tip construction and contour sampling. `.foam` files save the model inputs and view state; panel and control
solid bodies are regenerated when needed (projects.md and ADR-0006).

Processing reports outline sampling, section/tip construction, lofting, validation,
mirroring, meshing and display in the status bar with a wait cursor. Successful
completion and failures replace the progress message. Generation runs as an owned
background component job. Panel-qualified progress is drained on the GUI thread.
Up to four independent panel tasks run concurrently (bounded by available CPUs),
then assemble in order. Right-half display triangulations are mirrored with the
solid instead of computing the same left-half mesh again.

Within each panel, control/spar/split/hollowing Booleans and shape validation
use OCCT's per-operation parallel mode. Geometry and feature ordering are
unchanged. See regeneration.md for independent panel/kernel benchmark controls.

While processing, the menus, shortcuts, toolbars, data editors and viewports are
disabled. Only Cancel, at the bottom of the data panel, remains active. It requests
cooperative OCCT interruption and changes to Cancelling until all workers stop.
No partial results are displayed. Cancellation retains the previous display and
source data; re-entering 3D retries the current inputs. Closing the window requests
cancellation and completes once workers finish. Component jobs own their snapshots
and never access widgets or the viewer. See regeneration.md and ADR-0018.

ADR-0008 defines model input comparisons and camera retention.

Enabled aileron/flap rectangles split and bevel control bodies after the main
loft and before mirroring/meshing. The compound includes fixed and movable bodies
for both halves. See control-surfaces.md and ADR-0009.

Viewport lighting uses paired directional lights above and below the wing,
symmetric across the root mirror plane, plus ambient fill. Bottom and steep hinge
faces remain readable when the camera rotates underneath. This uses the existing
Phong renderer; ray-traced global illumination is not required. Lighting changes
do not alter project data, geometry, or the retained camera.

Enabled spars modify the fixed wing after control separation and before mirroring.
Per-panel Mid spars split only the fixed body at 50% local thickness and add alignment tabs
and matching holes. See spars.md and ADR-0012.

## Independent panel generation
With multiple panels, build a separate loft for each using only stations whose
LE/TE anchors belong to it. All use the global root-derived axes and full-wing
scale. Shape only the outermost tip; apply each panel's controls and spars before
assembling and mirroring. Different airfoil assignments at a panel join remain
distinct instead of being blended across it. See ADR-0013.

## Near-terminal stations on internal panels
When both ends of the last station lie within 1% of an internal panel's projected
span from its outermost outline point, generation ends at the inboard-most
projected endpoint of that station. This creates an equal-span end section and
removes the tiny traced overrun. Outlines and station anchors remain unchanged
in the editable/saved project. The original full-wing span still calibrates scale.
Stations farther inboard leave the panel extent unchanged. The outermost panel
is always exempt, retaining its complete outline and rounded wing-tip shape.

An oblique panel root uses a full airfoil cap joining the two open outline ends.
Intermediate constant-span sections start beyond that cap, avoiding an artificial
zero-chord section at its corner. Spar tools sample complete skin just beyond
the oblique cap and extend back through it; cutting against the solid preserves
the actual cap boundary. See ADR-0015.

An oblique station that touches or crosses the root cap uses the ordinary
full-chord section at its center span; adding its exact wire would overlap the
root cap. Its airfoil assignment remains part of interpolation.

With any nonzero dihedral, build panel sections with mitered root/end faces,
then rigidly rotate all panel bodies and place each root at the preceding tip.
Reflect the assembled right half at Y=0; both sides rise symmetrically. The root
miter lies on that plane. Zero-degree migrated projects keep their flat trace
placement. Angled joints use equal-span projected caps; each panel retains its
own chord and airfoil, so differing section definitions can create a step at a
joint even though their mating planes coincide. Matching definitions mate fully.

Optional Lightening creates stepped hollow bays only in the main wing, retaining
skins, spar/hinge clearances, uniform crossmembers and solid panel caps/tips. It
requires a main-wing access split regardless of Mid spar enablement. Controls
remain solid and unsplit. See lightening.md and ADR-0017.
