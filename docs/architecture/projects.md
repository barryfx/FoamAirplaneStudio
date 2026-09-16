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

Reference pages and airfoils are embedded, so external source files are optional
after saving. Generated solids are rebuilt from the saved inputs when Wing/3D
is active, using a cancellable background component job (regeneration.md). Menus
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
in all lifecycle operations. Version-1 and version-2 projects migrate on Save to version 8;
see control-surfaces.md and ADR-0009.

Version 3 adds spar options to the same snapshot, restoration and data/geometry
fingerprint paths. Versions 1/2 open with spars disabled; current saves
use version 8. See spars.md and ADR-0011.

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
