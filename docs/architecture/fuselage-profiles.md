# Fuselage profiles and solid generation

A Side View station or an existing profile enables Edit Profiles. Instructions appear at the top of
the data panel. Station numbers run nose to tail, matching Thicken rather than
file creation order. The selected station is orange and reveals Line/Spline/Circle controls,
with Circle below them. These three tools are mutually exclusive. Circle takes a
center click followed by a radius click and previews while the mouse moves.
Escape or switching tools discards a center-only circle. With tools off, dragging
the center moves the circle and dragging its radius point resizes it. Circles
persist as center/radius handles in format 28 and use the existing sampled-profile
pipeline. Formers has no new drawing controls.

The selected station's entire profile is orange with a white border, matching
the GUI selection palette. Copy picking and whole-profile dragging highlight the
chosen profile too. Other profiles remain blue; leaving Edit Profiles clears
the highlight. This display state is transient and does not modify the project.

Copy Profile arms a source picker: click any profile's boundary or closed interior
to store an independent session clipboard snapshot. Select a destination station
and Paste Profile to replace its profile with a copy. Shared source slots are
never overwritten. The copy is placed to the right of existing drawing bounds,
the canvas expands, and the viewport centers on it. Reopening includes profile
bounds in the canvas so Fit View and scrolling can reach pasted profiles. The clipboard survives mode
and project switches for the current application session and is not persisted.

Move Profile arms whole-profile dragging: press and hold the left mouse button
on any profile, drag, and release to drop it. All points translate together,
preserving circles, curves, dimensions and station assignments. Copy/Move and
drawing tools are mutually exclusive; Escape or leaving the mode ends the action.
Copy and action selection do not dirty the project; Paste and actual moves do.
Draw a closed cross-section anywhere on the 2D reference; all profile sketches stay visible in every 2D mode, while only the selected
station is editable. Turn tools off to select another station, drag points,
or Delete a curve. Delete Profile clears its entire sketch and assignment. Tool
and station selection alone do not dirty the project. Pending curves finish on
mode exit; an incomplete section remains editable and prevents solid generation.

Each ConstrainedLine has an optional profile slot independent of the Wing airfoil
index. A separate SketchEditor holds stable slots. Station moves and source
resynchronization preserve the slot; deleting another station cannot retarget it.
Deleting a station leaves its now-unreferenced slot in the saved collection.
In Edit Profiles, select an unassigned station, turn drawing tools off, and double-click
an orphaned profile boundary or closed interior to reattach that existing slot.
Single-click an unattached profile to highlight it, then press Delete or Delete
Profile to remove the entire sketch. Selection alone does not modify the project.
This also works after all stations are deleted; Undo/Redo restores/deletes the
sketch without creating a station.
The sketch does not move or duplicate. Occupied profiles and stations with an
existing assignment are unaffected. Recovery participates in project Undo/Redo
and persists through save/open.
Version 11 persists assignments, sketches and unfinished drawing. Earlier files
start unassigned. Both reference outlines and section sketches remap with the
reference image. New/Close clears the collection.

All stations must have closed profiles before Thicken, Cut, Servo Tray and Formers
become enabled. Horiz Stab and Vert Stab also become available immediately from
this definition readiness, without requiring a successful build or optional tab visits.
Their Outline, Airfoil and solid generation are implemented; Hinge Line is implemented; Cut supports closed material-removal shapes (stabilizer-solids.md). Thicken, Cut, Servo Tray and Formers are implemented (fuselage-thickness.md, fuselage-cuts.md,
servo-tray.md, formers.md). These readiness
checks certify loop connectivity and area; the solid builder also validates its
result and reports geometry failures in the status bar.

FuselageSolidBuilder takes immutable outlines, stations, profiles and optional
manual length. X runs from the leftmost nose to the rightmost tail. Each view's
longitudinal extent maps independently onto the Side View length derived from
the shared Wingspan calibration; in
actual-scale mode the Side View extent supplies the length in millimetres. Their
nose centers share Y=Z=0. Nearly vertical straight end edges receive the
model-only registration described in fuselage-thickness.md. A Side View station's normalized X identifies the
corresponding Top View section. Profile width and height fit independently to
those spans; height lies between the Side boundaries, retaining Side View
vertical offsets. The full Top View width is centred at Y=0 for the mirrored body.

For the initial loft, closed fitted boundaries are sampled. Winding is normalized
clockwise in drawing coordinates. Intersections with the
profile bounding-box centerlines define top, right, bottom and left landmarks;
16 arc-length intervals in each quadrant establish 64 corresponding samples.
Drawn up maps to fuselage up, regardless of curve order or drawing direction.
The right arc (samples 0 through 32, top/right/bottom) is authoritative and is
reflected to define the left arc. Thus asymmetrical traced left arcs do not
produce an asymmetrical model. The Top guide's total width is retained and
centred on the registered Y=0 plane; the saved sketches are unchanged.
Using the highest vertex as a seam is deliberately avoided: tiny roof slopes can
choose opposite corners at adjacent stations and twist the interpolated body.
Profiles whose directional centerline crossings cannot be ordered around a single
loop are rejected with an explanatory error. Shapes interpolate between stations;
the nearest assigned shape extends beyond the first/last station. Both view
outlines act as longitudinal guides for width, height and centerline
position. All station positions and 33 baseline positions are retained. Additional
sections follow guide curvature and narrow shoulders: all sampled outline X
breakpoints are examined, and intervals subdivide until linear interpolation of
each of the four rails deviates by at most 0.1 mm from its sampled guide. This is
rail interpolation tolerance, not an analytic surface or manufacturing guarantee.
An OCCT ruled solid loft closes the right half, including its mating plane. Point-shaped ends use vertices; line-shaped ends use a
0.0001 mm finite cap. One assigned profile is sufficient. The complete Fuselage generates an inward wall using saved/default station
values without requiring a Thicken visit
(fuselage-thickness.md). The sampled loft is not an exact analytic skin. Sampling may soften
small profile corners and approximates curved silhouettes between sections.
Ambiguous vertical crossings, interior pinches, invalid lofts and zero volume
are rejected. Export/manufacturing tolerance controls remain future work.

MainWindow owns a separate Fuselage BackgroundJob, fingerprint and shape cache.
Only the GUI thread touches widgets/viewport; the worker builds and meshes its
own OCCT shapes. Existing Cancel, processing lock and project-epoch protections
apply. The UI permits one component job at a time, with independently owned
workers and cached results. Navigating back to an unchanged Wing displays its
cache without regeneration; Fuselage data is excluded from Wing's fingerprint.
Status messages report preparation, alignment, lofting, meshing, completion,
failure and cancellation. Stabilizers own independent builders and jobs (stabilizer-solids.md).

The shared builder also parallelizes independent wall offsets and enables OCCT
parallel Boolean, validation and meshing work. Loft/cavity/accessory/cut ordering
is preserved. See regeneration.md for the per-operation serial comparison option.

Generation now hollows, adds supports/rails and cuts stiffener grooves in the
right half, then reflects
it as a separate left part. The main halves are never joined. Whole cavity tooling
fits the removable formers and tray. Holes, cuts and alignment pins/sockets are
applied afterwards; detached cut-out pieces retain whole-part behavior. See
ADR-0045 and the half-fuselage benchmark report.
