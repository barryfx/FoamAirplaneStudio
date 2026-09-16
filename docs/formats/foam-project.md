# FoamAirplaneStudio project format, version 8

Files use `.foam`, UTF-8 JSON, `format: "FoamAirplaneStudio"`, `version: 8`.
All lengths ending in `Mm` are millimetres. Sketch coordinates remain scene
coordinates (pixels in manual-reference mode; millimetres in actual-scale mode).

| Field | Meaning |
| --- | --- |
| reference | Original filename, embedded ordered PNG pages, per-page/combined physical size, native/project units, scale mode, full wingspan/fuselage length and their text fields |
| wingOutline | Numbered wing-panel sketch layers, active layer, current tool, selected curve, pending points and editing flag |
| airfoilSketches | Independent airfoil trace layers and their editing state; never interpreted as wing planform outlines |
| stations | Ordered LE/TE anchors, alignment, optional airfoil library index, selected station and optional pending first anchor |
| airfoils | Named imported profiles or closed trace snapshots, ordered boundaries, current choice and named unfinished draft |
| controlSurfaces | One two-control array per panel, selected panel, hinge/rectangle settings and pending drawing state |
| spars | One array per outline panel, each containing Top/Bottom/Mid spar records |
| lightening | Whole-wing enabled flag, physical wall/rib/setback lengths, crossmember count and entered dimension text |
| dihedralDegrees | One relative root angle in degrees per wing panel, ordered root to tip |
| ui | Workspace index, active toolbar label, viewport tab, 2D zoom/center, optional 3D camera and splitter sizes |

Sketch layers contain `[x,y]` points and curves referencing zero-based point
indices. Curve/tool values are 0 None (tool only), 1 Line, 2 fitted Spline.
Station anchors contain a layer index, curve index, displayed arc-length fraction
from 0 through 1, and current scene position. Alignment values are 0 Free,
1 Horizontal, 2 Vertical. Station airfoil indices refer to the ordered shared
library. Imported coordinates are normalized TE/LE/TE contours; traced airfoil
boundaries remain screen-oriented and are normalized during solid generation.

Reference units are 0 millimetres or 1 inches. Optional physical sizes and lengths
use JSON null. Each page stores `png` (base64 lossless PNG) and optional
`physicalMm: [width,height]`. The saved filename is provenance, not a required
external dependency. The original reference is not reread on Open.

UI workspace indices follow the primary toolbar; viewport is 0 2D or 1 3D.
2D state contains positive zoom and scene center. Camera fields are eye, center,
up (three-element vectors), scale, vertical field of view and OCCT projection
type (0 orthographic, 1 perspective, 2 stereo, 3/4 mono stereo eyes).
Source state drives toolbar readiness and generated solids when restored.

The reader validates types, finite ranges and cross-references before applying
anything. The reader limits files to 512 MiB, embedded references to 1,000 pages
and 200 million total pixels, wing panels to 100, airfoil entries to 10,000,
and individual point arrays to one million. Qt image-decoder allocation limits
also apply. Unsupported versions, malformed JSON/PNGs, bad anchors, invalid
profiles and invalid cameras fail without replacing the current project.

Save writes a temporary sibling through QSaveFile and atomically replaces the
destination after successful completion. No autosave or legacy `.designrc`
migration is implied. See ADR-0006 for ownership and restoration decisions.

View and editor-selection state is serialized in version 8, but is excluded
from the unsaved-data comparison. An explicit Save captures current navigation;
view-only changes never require Save/Discard on closing (ADR-0008).

Version 2 adds `controlSurfaces: {surfaces: [...], drawing: -1, first: null}`.
`surfaces` contains exactly two entries: ailerons, then flaps. Each has `enabled`
(boolean), `hinge` (0 Tape, 1 Standard), and `rectangle` (null or `[x,y,width,height]`
with finite coordinates and positive dimensions in scene units). Disabled controls
retain their rectangles/settings. `drawing` is -1 idle, 0 ailerons, or 1 flaps;
`first` is an optional `[x,y]` first corner. Active drawing requires an enabled
surface and Wing/Ailerons-Flaps mode. Malformed settings are rejected before Open
mutates the current project. Version 1 still opens with both controls disabled;
Version 2 introduced these fields (ADR-0009); current Save/Save As writes version 8.

Generated control surfaces use a fixed 1/16-inch (1.5875 mm) clearance at each
spanwise rectangle end, on the moving body only. This is a generation rule, not
an editable project field; version 2 rectangles and hinge settings remain
unchanged. Opening an existing project regenerates its controls with this rule.

Leaving control-surface editing or selecting 3D clears enabled flags whose
rectangles are null. The cleared flags are ordinary project changes and are
preserved by Save/Save As. A pending drawing may still be restored when opening
a project; restoration itself does not discard the saved draft.

Version 3 adds a required `spars` array with exactly three objects, ordered Top,
Bottom, Mid. Each stores `enabled` (boolean), `shape` (0 Round, 1 Strip; Mid must
be 0), `chordPercent` (0..100 from local LE), `lengthPercent` (0.01..100 of
half-span from root), `sizeMm` (round diameter or strip width) and `heightMm`
(strip inward depth). Physical sizes must be finite, positive and at most
10,000 mm. Unused and disabled values remain stored. Alignment geometry uses
the generation rules (updated by ADR-0012) rather than independent user-editable fields.
Version 1/2 readers in this app initialize spars disabled with default dimensions;
Version 3 introduced these records; version 4 changed their layout as described below. No generated solids
are serialized: spar cuts, split and tabs are regenerated from these inputs.

Version 4 makes `spars` an array of panel arrays. Its count must equal the outline
panel count (1..100), and every panel must contain exactly three records with the
same fields/ranges as version 3. Length percentage is now relative to that panel.
`ui.sparPanel` stores the zero-based selected spar tab and is excluded from data
and geometry fingerprints. It must reference an existing panel.
Versions 1/2 initialize all panels disabled. Version 3 loads global spar settings
into Panel 1, with others disabled. Current Save/Save As writes version 8; older
applications reject it. Mid now uses 50% local thickness and a matching split
surface (ADR-0012), also when regenerating migrated projects.

Overlapping enabled aileron/flap rectangles remain loadable for correction. They are a geometric validation error, reported in the control panel and rejected before wing generation, rather than a malformed file. No overlap flag is serialized.


Version 5 replaces `controlSurfaces.surfaces` with `controlSurfaces.panels`, an
array matching the outline count. Each element is the former two-control array.
`controlSurfaces.panel` selects the zero-based owner of any pending first corner.
`ui.stationPanel` and `airfoils.panel` select their respective panel tabs.
`airfoils.panelChoices` holds one library index (or -1) per panel. These choices
and idle selected tabs are excluded from data/geometry fingerprints. A pending
control corner retains its panel in the data fingerprint to preserve meaning.
Station anchors identify ownership without a new station index field. Both
anchors of newly drawn stations belong to the same outline panel.

Versions 1�4 preserve their shared library and stations. Former global controls
load into Panel 1, with other panels disabled. Legacy cross-panel stations are
retained for correction; each panel must have its own valid LE/TE pair before
completion/generation. No automatic duplication of controls or assignment to
new stations occurs during decoding. Versions 1�4 apps reject version-5 files.

## Version 6: mixed-unit dimension presentation

Each spar adds `sizeText` and `heightText`, preserving the user's decimal and
explicit unit suffix independently of `sizeMm` and `heightMm`. Empty strings
mean an untouched/default display in project units. Nonempty strings must parse
to the corresponding canonical millimetre value (relative tolerance 1e-8).
The reader accepts bare strings using saved project units and normalizes them
with an explicit suffix. Versions 1-5 initialize these strings empty.
Reference `wingspanText` and `fuselageText` already store raw entry text; they now
allow explicit mm/in suffixes. Canonical millimetre dimensions remain unchanged.
Display text participates in project dirty detection, but spar display text is
excluded from the geometry fingerprint. Versions 1-5 remain readable; older apps
reject version-6 files. See ADR-0014.

## Version 7: per-panel dihedral and implicit rounded tips

`dihedralDegrees` is an array matching the outline panel count (1..100). Every
entry is a finite degree value from -80 through 80. The first is relative to
horizontal; later entries increment the previous panel's angle. Cumulative angles
outside (-85,85) are a generation error, retaining editable saved input.
`ui.dihedralPanel` is the zero-based selected tab. Angles affect project/geometry
fingerprints; tab selection affects neither. New panels default to zero.

The writer no longer emits `wingTip`. The reader accepts versions 1-6, validates
their legacy tip enum when present, ignores that choice, initializes all panel
angles to zero and maps a Wing workspace `Wing Tip` tool to `Dihedral`. Existing
reference, outlines, stations, spars, controls and view data are retained. The
outermost tip is always rounded, including for migrated files. Older apps reject
version 7. Original files are only upgraded when the user saves them.

## Version 8: wing lightening

`lightening` is an object with `enabled` (boolean), `wallMm`, `ribMm` (positive,
0.0001-10000 mm), `startMm`, `stopMm` (0-10000 mm), `crossmembers` (integer 0-100),
and `text` (exactly four strings: wall, rib, start, stop). Default values are off,
2, 3, 25, 25 mm and four crossmembers. Empty text means default presentation in
project units. Nonempty strings must parse to their matching millimetre value,
within relative tolerance 1e-8, and preserve an explicit unit suffix on load.
Start/stop text accepts zero. Unsupported types/ranges or inconsistent display
text reject Open transactionally. Geometric range/pitch conflicts remain editable
saved input and are reported during generation.

Versions 1-7 initialize Lightening disabled. Current saves write version 8; older
apps reject it. No source file changes until Save. All project lifecycle paths
preserve these settings; New resets defaults. Text participates in dirty checking
but is omitted from the model fingerprint. The current Lightening toolbar mode
is captured by the existing UI tool field. See lightening.md and ADR-0017.
