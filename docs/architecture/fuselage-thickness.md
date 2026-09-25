# Fuselage wall thickness

All profile sketches are painted across all 2D modes, including unselected
profiles. Editing handles still belong only to the active Edit Profiles station.

Opening 3D with complete outlines/profiles initializes missing wall defaults and
enables hollow generation, whether or not Thicken has been visited. Opening
Thicken also initializes these defaults. Existing explicit values are preserved. The data panel shows instructions and a decimal length field
for each station, numbered and listed from nose to tail (ascending Side View X).
Edit Profiles uses the same display numbers. File/storage order is unchanged;
sketches and wall values remain attached to their original station records.
Fields display the Reference
project units; bare numbers use those units, while explicit `mm` or `in` overrides
are accepted. Committed entries redisplay in project units, with millimetres stored
in the existing project format. Changing display units never changes the physical
thickness. Invalid or out-of-range entries revert to the stored value.
Initialization uses 8 mm for every station at or forward of the wing leading edge and 5 mm
aft of it. A nearly horizontal straight upper Side View edge whose length agrees
with the Wing root chord within 20% is preferred as the wing seat. Its forward
end locates the LE. The nearest station within 2% of the root chord is registered
as the LE station, accommodating small tracing offsets. The boundary is inclusive:
that station and every forward station receive 8 mm; only aft stations receive 5 mm.
If none matches, the root Wing station's reference X is used;
the chosen reference X is displayed. GentleLady's confirmed wing-seat edge begins
at X=253.173265724253; its nearby station at X=256.3825345142255 is registered
as the LE station. Its nose-to-tail defaults are therefore 8/8/5 mm. These defaults are initial values; moves and later reference
changes never overwrite an entered thickness. Newly added stations receive a
default when thickening is enabled.

Each ConstrainedLine owns its optional thickness. This survives movement, source
resynchronization and profile replacement/deletion. Removing a station removes
its thickness. Thicken activation persists through mode changes. Version 12 saves
the activation and values; versions 1-11 start disabled. New/Close resets them.
Dirty checks and the Fuselage fingerprint include thickness, while Wing's do not.

The right-half outer guided loft is retained. At each longitudinal section, OCCT planar
offsets move the section inward by the interpolated wall distance. These values
specify offsets in the cross-section plane, rather than a global 3D surface-normal
offset. A monotone cubic smoothstep law passes through station values with zero
slope at stations and no overshoot; the nearest value is used beyond the outermost
stations. Additional section samples capture the transition. Inner loops use the
same drawn-up landmark correspondence as the outer profiles. The sampled ruled
loft approximates the smooth thickness law, without discrete thickness steps.

Nose and tail follow the same rule independently. An end is open when the nearest
profile lies at the corresponding registered outline endpoint. Model-only end
registration makes an explicit straight end edge vertical when it is within 2
degrees of vertical and its axial drift is at most 0.5 mm at the final model scale.
The outermost station within 0.5 mm of such a plane is registered onto the plane
and opens that end. Interior stations keep their positions. Curved, pointed and
more strongly sloping ends retain the strict 1e-6 mm endpoint test. Both views
use the corrected end midpoint for alignment, including reference/part placement.
The drawing and saved project inputs are unchanged. See ADR-0044.
An annular rim joins the outer and inner skins there. If the outlines extend past
the outermost profile, that end stays closed: an axial end wall is retained and
narrow tips remain solid. A station terminating an outline must leave room for
its requested wall and opening, otherwise generation reports an error.

The right-half inner wall is lofted and subtracted from the right outer solid.
The cavity reaches the end plane for an open end, leaving an annular half-rim;
closed ends retain axial material. The completed half is reflected as a separate
part. Whole-cavity tooling is assembled only for inserts and holes. The input
sketches remain unchanged. The full-symmetric benchmark comparison path retains
the prior shell/rim assembly procedure. The result must pass OCCT validity and positive-volume checks
and contain less material than the exterior solid. A thickness that eliminates
the cavity or pinches it off inside the body is rejected. Sampling and planar
offset assumptions are explicit; manufacturing surface-normal thickness controls
remain a future refinement.

Generation stays in the independent Fuselage worker with existing cancellation,
stale-result and cache protections. Status reports cover offsets, inner loft,
closed tips, failures and hollow-body completion.

When OCCT's planar Intersection offset fails without a result on a sampled
polygon, a checked line-offset fallback intersects the inward-shifted edges and
removes consumed convex edges. It rejects reversed concave edges, intersections,
nonfinite/degenerate edges, and any edge violating the original wall clearance.
The normal inside/area checks still apply. A successful kernel result containing
multiple or no loops is not replaced by this fallback. This handles the mirrored
BabyBuzzard corner sampling failure without reducing thickness or bridging a
collapsed neck. `insetFuselageSection` exposes this numeric section operation
for focused tests without generating a fuselage.
