# Airfoil library and station assignment

Wing / Airfoils displays the requested instructions, Load Airfoil .dat File,
Sketch Airfoil, and a scrollable radio list titled Airfoil for selected station.
The shared file chooser remembers its last selection with purpose key airfoilDat.
Names come from the DAT header when present; coordinate-only files use the file
basename. Selig contour and Lednicer two-surface DAT layouts are supported.
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
