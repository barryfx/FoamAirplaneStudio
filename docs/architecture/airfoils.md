# Airfoil library and station assignment

Wing / Airfoils displays the requested instructions, Load Airfoil .dat File,
Sketch Airfoil, and a scrollable radio list titled Airfoil for selected station.
The shared file chooser remembers its last selection with purpose key airfoilDat.
Names come from the DAT header when present; coordinate-only files use the file
basename. Selig contour and Lednicer two-surface DAT layouts are supported.
Export Selected Airfoil .dat saves the checked library entry through the shared
save dialog (purpose `airfoilDatExport`). It supports imported and traced profiles,
using 35 cosine-spaced samples per surface (69 coordinate rows maximum, excluding
the name header), ordered from
upper TE through LE to lower TE. LE X is 0 and both TE X values are 1; a blunt
trailing edge retains its two heights. Traces use image Y inversion and remove the
LE-to-TE-midpoint baseline. The name is written as the DAT header and numbers use
decimal points independent of locale. Saving is atomic. Export is disabled for
an empty library or during tracing and does not modify profiles or assignments.

Smooth Airfoil opens a modal comparison of the checked entry. Blue dashed lines
show the original and orange shows the result at equal X/Y scale. A 0–100 strength
slider defaults to 25; larger values penalize local curvature more strongly.
Maximum thickness and signed maximum camber are displayed as percentages of chord.
Save Copy adds a named imported-profile entry (default “Name — Smoothed”), leaving
the original trace, selection and station assignments unchanged. Select the new
entry to assign or export it. Cancel has no project effect. The button is disabled
while tracing or without a selected entry. Copies participate in ordinary project
history and input-only save/load; no format change is required.

The fitter operates on 161 cosine samples per surface, using 16-control clamped
cubic B-splines for camber and half-thickness and a second-difference penalty.
Endpoint baselines plus an x(1-x) camber envelope preserve LE and both TE heights.
For a contour that repeats its starting TE point, a short, nearly vertical final
closing segment is excluded from the surface fit. Detection requires a segment
within the last 0.5% chord, at least 0.1% chord in height, steeper than 2:1 and
ten times the adjacent surface slope. The adjoining surface tangent extends to
X=1 to recover its own TE endpoint. This handles either traversal direction and
exactly vertical caps; ordinary sharp or already-separated blunt TE endpoints
remain unchanged. The preview closes the result with a separate straight line.
The dialog starts at 1000 by 680 pixels and can be resized.
The sqrt(x)(1-x) thickness envelope gives a common vertical nose tangent;
positive thickness coefficients prevent interior crossings for the entire fitted
curve. Existing crossed surfaces and zero-thickness input are rejected. A small
positive coefficient floor supplies a rounded nose even for a pointed trace.
The fitted curve is stored as a 321-point contour; DAT export still uses 69 points.
Normalization is shared with DAT export. Maximum thickness/camber are reported,
not locked; this is geometric smoothing, not aerodynamic optimization.
Empty, underspecified, zero-chord, zero-thickness or otherwise unusable imports
show an error and leave the library unchanged. Nonzero leading-edge X coordinates
are normalized using an unchanged source minimum across every input point.

Entering Airfoils automatically selects the first station (or preserves a valid
selection). Escape and empty-space clicks do not clear it. The station stays
highlighted during airfoil tracing, while assignment interaction is suspended.
The first entry becomes the default radio selection. Clicking an unassigned
station assigns the currently selected airfoil. Clicking an assigned station
restores its own radio selection. Clicking a radio changes only the selected
station. One station is
highlighted at a time. Station endpoints and outline geometry are locked in this
mode, including Delete. All station assignments must exist before Dihedral is
enabled. The library and controls remain shared when switching stations.

Sketch Airfoil asks for a name and stays lit while active. Canceling the name
dialog (or providing an empty name) does not enter sketch mode. Load, description
and assignment list are disabled/gray; Sketch Airfoil and its temporary
Line/Spline buttons remain enabled so users can draw and exit. Line/Spline are
mutually exclusive toggles. Curves snap to existing trace points using the normal
screen tolerance. A spline may close by clicking its first point after at least
three distinct points. Multiple lines/splines may form the loop in any order.

Pressing Escape or Sketch Airfoil again finishes the pending curve and exits
airfoil sketching. Exactly one connected, closed, unbranched loop of nonzero
area is required. Open, empty, branched or multiple loops show a warning, but
exit proceeds and the invalid draft is discarded. Valid traces enter the list
under their supplied names and remain visible in 2D. Leaving Airfoils via another
toolbar button applies the same finalization rule.

Sketch layers for airfoils are separate from wing layers. No wing or station
geometry is changed by tracing. Library entries preserve imported profiles or
trace snapshots and ordered boundaries; station records retain their chosen
library index. New clears them. They survive workspace changes within the running
project session and are saved in `.foam` files, including named unfinished drafts.
See ADR-0004 and the persistence extension in ADR-0006.

Assigned profiles now drive the mirrored 3D wing (wing-solids.md and ADR-0005).
Draw traced airfoils with LE left and TE right; the builder normalizes their
horizontal chord and removes the LE-to-TE baseline before interpolation.

## Panel assignment
Numbered tabs select the panel whose stations can be assigned. Entering a panel
selects one of its stations. Each station's assignment and each panel's current
choice are retained separately; DAT imports and valid sketches populate the one
shared library immediately. Tracing disables panel tabs until finalized. Every
panel needs two stations and all assignments before downstream tools unlock.
Panel lofts use only their own stations; matching profiles at joins are the user's
choice. See ADR-0013 and project format version 5.
