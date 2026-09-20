# Project lifecycle and restoration

File > New creates an empty Reference project. Open reads `.foam` files. Save
writes the current filename (or asks for one); Save As writes a new file and
adopts that filename. Close Project clears and disables the editing area while
keeping the app open. New/Open remain available. Standard platform shortcuts
are assigned to these actions.

New, Open, Close Project and app exit prompt Save/Discard/Cancel if project data
changed. Cancel or a failed save leaves the current project intact. Invalid Open
files are rejected before this prompt, so they never discard the current state.
The window title shows the filename/Untitled and a modification marker.

ProjectDocument captures all current models and UI state; ProjectDocument.cpp
encodes, validates and atomically writes the versioned format. MainWindow applies
validated snapshots while suppressing intermediate workflow callbacks. It loads
the reference and editor data, recomputes readiness, restores the selected modes,
then reapplies pending drawing state, camera and 2D view. This prevents toolbar
activation from finishing saved drafts or overwriting selections. A clean data
fingerprint is recorded immediately, independently of queued layout events.
Restored 2D view bounds use the maximum viewport size, independent of scrollbar
visibility. Using the current inner viewport size can cause scrollbars to
oscillate on saved zoom/center restoration and overwhelm the UI event loop.

Reference pages and airfoils are embedded, so external source files are optional
after saving. Generated solids are rebuilt from the saved inputs when the selected component
is ready and 3D is active, using a cancellable background component job (regeneration.md). Menus
and editing are disabled until completion; Cancel remains available. Hover markers and mouse-button capture are transient; the latest
point coordinates, pending sketch points and first station anchor are retained.
See ../formats/foam-project.md and ADR-0006.

Reset explicitly recomputes Reference eligibility while restoration callbacks are
suppressed: only Reference is enabled in a new empty project. Reading/validation,
restoration and saving show a scoped wait cursor and status messages. Dialogs for
file selection and unsaved changes retain normal interaction. The same processing
scope is used for reference image/PDF and airfoil loading; errors unwind the cursor.

Modes, selections, camera, zoom, scroll and splitter positions remain in saved
files but do not count as unsaved data changes. Draft geometry and entered values
do count. Explicit Save writes both data and the latest view state. See ADR-0008.

Control-surface flags, hinge choices, rectangles and drawing drafts are included
in all lifecycle operations. Version-1 and version-2 projects migrate on Save to version 25;
see control-surfaces.md and ADR-0009.

Version 3 adds spar options to the same snapshot, restoration and data/geometry
fingerprint paths. Versions 1/2 open with spars disabled; current saves
use version 25. See spars.md and ADR-0011.

Version 4 stores spars per outline panel and the selected spar tab. The tab is
view state; the per-panel arrays are data/geometry inputs. Version-3 global
settings migrate into Panel 1. See ADR-0012 for migration and half-thickness split.

Version 5 adds panel control arrays, Airfoil Stations/Airfoils/Ailerons-Flaps tab
selection and per-panel library choices. The library remains shared and station
anchors retain ownership. Versions 1–4 are readable; old global controls load
into Panel 1 without duplication. See ADR-0013 and formats/foam-project.md.

Version 6 retains per-field mixed-unit spar text and existing reference text.
Geometry still consumes millimetres; equivalent presentation changes do not
rebuild the wing. See ADR-0014 and the format specification.

Version 7 adds per-panel dihedral angles and the selected Dihedral tab. Old tip
choices are read for compatibility but no longer retained; generation always
rounds the outer tip. Versions 1-6 receive zero angles. See dihedral.md.

Version 8 adds whole-wing Lightening fields and their mixed-unit presentation.
Older projects open with lightening disabled; New resets it. Changes invalidate
geometry, while navigation and equivalent unit presentation do not. See the
project-format reference and lightening.md.

Version 9 adds independent Top/Side fuselage outlines and active view/tool/drafts.
Versions 1-8 load empty fuselage outlines. New/Close clears them; Open and Save
restore their geometry and view state. See fuselage-outlines.md and ADR-0019.

Version 10 adds independent vertical fuselage profile stations on Side View.
Versions 1-9 load an empty collection. See fuselage-stations.md and ADR-0020.

Version 11 adds station-linked fuselage profile sketches, including drafts. Versions
1-10 load unassigned profiles. See fuselage-profiles.md and ADR-0021.

Version 12 persists first-entry Thicken activation and each station thickness.
Older projects start with thickening disabled; see fuselage-thickness.md.

Version 13 adds persistent Top/Side cut sketches, including unfinished paths.
Older files load empty cuts; generated body compounds remain transient. See
fuselage-cuts.md and ADR-0023.

Version 14 persists Servo Tray placement (legacy unfinished corner entry is cleared on restore). Versions
1-13 load no tray; generated tray bodies/top faces remain transient.

Version 15 adds persistent Formers rectangles and next thickness, with overlap validation.

Version 16 adds independent horizontal/vertical stabilizer open outlines and drafts. Older projects load empty outlines; removed stabilizer tool names migrate. See stabilizer-outlines.md.

Version 17 embeds each stabilizer airfoil selection, with bundled NACA009 defaults for versions 1-16. Stabilizer solids rebuild from outline, airfoil and reference scale.

Version 18 persists each stabilizer leading-edge endpoint; older projects require an explicit choice before generation.

Version 19 adds independent stabilizer hinge sketches/drafts and Tape/Standard choices.

Version 20 adds per-stabilizer closed Cut Shape collections and drafts.

Version 21 adds Assembly X/Z placements and cut intent. Explicitly selecting the
Assembly tab activates its 3D-only view, prepares missing source models and
reapplies requested cuts. Opening a project follows the 2D behavior below.
Original and cut solid caches remain transient; see assembly.md.

Opening a project always selects 2D View, regardless of its saved viewport, so
opening alone never regenerates models. A saved Assembly workspace opens in
Fuselage/Outline/Side View instead, preserving Assembly placements and cut intent.
Explicitly selecting 3D View or entering Assembly starts model preparation.

Version 23 saves design inputs and Assembly placement/cut intent only. All generated
geometry remains in session caches and is rebuilt on explicit 3D/Assembly entry
after Open. The reader accepts version 22 but ignores its entire `models` field,
without BREP decoding, decompression or cache validation. Resaving removes that
obsolete field. Saving never clears the current session caches. See ADR-0035.

Version 24 adds per-former Rotation Angle in degrees about the Side View mask
center; negative angles rotate counter-clockwise. Earlier projects load zero
angles. See formers.md and ADR-0036. Current saves use version 25.

Export uses session Assembly geometry (export.md). Saving from Export retains
that workspace choice, but opening returns to Fuselage Side View in 2D, as for
Assembly. Output selections/formats are transient; the directory is remembered
in QSettings independently of the project.

Version 25 adds four fuselage hole layers (Top/Bottom/Left/Right), including
drafts. Holes participate in input-only persistence, remapping, dirty checks and
fuselage/Assembly invalidation. Older projects load empty holes. Cut retains its
existing two layers while providing whole connected-path selection and deletion.
