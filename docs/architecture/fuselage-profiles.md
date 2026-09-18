# Fuselage profiles and solid generation

One Side View station enables Edit Profiles. Instructions appear at the top of
the data panel. Station numbers run nose to tail, matching Thicken rather than
file creation order. The selected station is orange and reveals Line/Spline controls.
Draw a closed cross-section anywhere on the 2D reference; all profile sketches stay visible in every 2D mode, while only the selected
station is editable. Turn tools off to select another station, drag points,
or Delete a curve. Delete Profile clears its entire sketch and assignment. Tool
and station selection alone do not dirty the project. Pending curves finish on
mode exit; an incomplete section remains editable and prevents solid generation.

Each ConstrainedLine has an optional profile slot independent of the Wing airfoil
index. A separate SketchEditor holds stable slots. Station moves and source
resynchronization preserve the slot; deleting another station cannot retarget it.
Deleting a station leaves its now-unreferenced slot in the saved collection.
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
longitudinal extent maps independently onto the manual Fuselage Length; in
actual-scale mode the Side View extent supplies the length in millimetres. Their
nose centers share Y=Z=0. A Side View station's normalized X identifies the
corresponding Top View section. Profile width and height fit independently to
those spans; the width is centered between the Top boundaries and height lies
between the Side boundaries. Offsets of each outline's centerline are retained.

For the initial loft, closed fitted boundaries are sampled. Winding is normalized
clockwise in drawing coordinates. Intersections with the
profile bounding-box centerlines define top, right, bottom and left landmarks;
16 arc-length intervals in each quadrant establish 64 corresponding samples.
Drawn up maps to fuselage up, regardless of curve order or drawing direction.
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
An OCCT ruled solid loft closes the ends. Point-shaped ends use vertices; line-shaped ends use a
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
