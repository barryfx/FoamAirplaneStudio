# FoamAirplaneStudio project format, version 33

Version 33 adds `stiffeners`: `count` (integer 0–16 per side), `shape`
(Round=0, Strip=1), `startPercent` and `stopPercent` (0–100, start < stop),
and `widthMm`, `heightMm`, `diameterMm` (0.01–100 mm). Counts are mirrored on
both fuselage sides. Defaults are disabled (count 0), Strip, 50–95%, width
3 mm, height 1 mm and diameter 3 mm. Versions 1–32 initialize these defaults.
Unused shape dimensions remain saved for switching shapes. Generated grooves
and stock records stay in session caches; optional derived statistics retain
their existing cache rules. No geometry is saved. Older readers
reject version 33. See ADR-0052 and architecture/fuselage-stiffeners.md.

Version 32 adds `fiberglass`, an array ordered Wing, Fuselage, Horiz Stab, Vert
Stab. Each entry has `sketch` (the usual layered sketch state, including Circle)
and a matching `patches` array. Every patch stores nonempty `name`, `wrap`,
`side` (Top=0, Bottom=1, Left=2, Right=3), positive `clothGm2`, `imperialCloth`,
`automaticResin`, optional `projectClothUnits` (default true), and positive `resinThicknessMm`. The latter is the manual
override; automatic resin thickness is derived from cloth weight.
When `projectClothUnits` is true, display follows Reference units; otherwise
`imperialCloth` selects the explicit override. Older version-32 records without
the optional flag follow project units. Physical cloth weight stays in g/m². Each component
has 1–1000 layers/metadata entries; empty initial layers carry no material.
Horizontal components accept Top/Bottom, Vert Stab accepts Left/Right, and
Fuselage accepts all four. Open drafts are valid saved inputs; coverage is
validated when measuring. `weightBalance.resinDensityKgM3` defaults to 1500 kg/m³ (1.5 g/cm³).
Versions 1–31 initialize empty patches and that density. Derived statistics
balance records optionally contain fiberglass area, cloth mass, resin volume and
centroid entries, covered by the new mass-source key. See ADR-0050.

Version 31 expands `fuselageCuts.layers` to four entries: Top, Bottom, Left,
Right. Active view indices use the same order. Versions 13–30 migrate layer 0
to Top and layer 1 to Left; Bottom and Right start empty, and active Side becomes
Left. Versions 1–12 initialize four empty layers. All saved end booleans remain
unchanged; new projects default Nose Open and Tail Closed. Earlier readers reject
version 33. The user-facing surface cut rule applies after migration too.

Version 30 adds `fuselageNoseOpen` and `fuselageTailOpen`. Boolean values select
ends independently; version-30 new projects defaulted to Closed. Versions 1–29 load with
legacy automatic inference (represented internally and in headless round trips
by null). The GUI resolves these legacy values from the existing stations once
on load, displays the corresponding radio buttons, and saves explicit booleans.
Subsequent station/outline changes do not silently toggle the selections. These
settings are included in undo history and fuselage/assembly/mass cache keys.
Older readers reject version 30 rather than silently changing the end geometry.

Version 29 adds `insideDiameterMm` and `insideDiameterText` to each spar record,
and `carbonFiberDensityKgM3` to `weightBalance`. Mid ID is finite, nonnegative
and strictly less than OD (`sizeMm`); zero denotes a solid rod. Empty ID text
means the default follows max(0, OD minus 1 mm) on subsequent OD edits. Explicit
text must parse to the stored millimetres. CF density is finite, 0.001–10000 kg/m³,
default 1540. Older projects retain their OD and initialize Mid ID to max(0,
OD minus 1 mm), other IDs to zero, and CF density to 1540. New panels default
to 6 mm OD / 5 mm ID; Top/Bottom defaults remain unchanged. Spar material
per-spar volumes/centroids remain transient; aggregate material measurements may be saved in the statistics cache. Older readers
reject version 29 rather than silently losing the new material settings.

Version 28 adds Circle (`type: 3`) only to `fuselageProfiles` curves and tools.
Each circle references exactly two distinct points: center and radius handle,
with a finite positive radius. Pending Circle input contains at most the center.
Other sketch collections continue to accept only Line/Spline. Versions 1–27 remain
readable and cannot contain Circle. The two handles are independent of other curves.

Version 27 adds `componentNames`, an object mapping source component identifiers
to user-visible export names, and workspace 8 (Inspect). Names are 1–120 characters
and valid as cross-platform filename stems. Identifiers are at most 240 characters;
at most 10,000 overrides are accepted. Versions 1–26 load an empty name map.
Inspect checkbox visibility remains session-only. See ADR-0041.

Files use `.foam`, UTF-8 JSON, `format: "FoamAirplaneStudio"`, `version: 33`.
All lengths ending in `Mm` are millimetres. Sketch coordinates remain scene
coordinates (pixels in manual-reference mode; millimetres in actual-scale mode).

| Field | Meaning |
| --- | --- |
| weightBalance | Foam, Aero Plywood and Carbon Fiber densities and named parts with physical dimensions, masses and X/Z centers |
| componentNames | Persistent export-name overrides keyed by source component identifier |
| airplaneStatistics | Optional derived dimensions and validated aggregate material measurements; no geometry |
| reference | Original filename, embedded ordered PNG pages, per-page/combined physical size, native/project units, scale mode, full wingspan and its text; legacy fuselage length/text fields retained for compatibility |
| assembly | Positioned flag, three physical X/Z translations, three root-pivot rotation angles and cut/uncut intent |
| stabilizerAirfoils | Two entries in horizontal/vertical order: null for bundled NACA009, or embedded DAT name and normalized coordinates |
| horizontalStabilizerOutline / verticalStabilizerOutline | Independent single-layer open sketches, tools, selections and pending points |
| horizontalStabilizerHinge / verticalStabilizerHinge | Independent single-layer connected Line sketches and drafts |
| stabilizerHingeCuts | Two hinge styles in horizontal/vertical order: 0 Tape, 1 Standard |
| horizontalStabilizerCuts / verticalStabilizerCuts | Independent collections of closed Cut Shape sketch layers, tools, selections and drafts |
| wingOutline | Numbered wing-panel sketch layers, active layer, current tool, selected curve, pending points and editing flag |
| formers | Positive Side View rectangles, per-former rotationDegrees and next-former thicknessMm |
| servoTray | Side View rectangle [x,y,width,height], optional first corner, and drawing flag |
| fuselageCuts | Four cut sketch layers in Top/Bottom/Left/Right order with active view/tool, selection, editing and pending points |
| fuselageNoseOpen / fuselageTailOpen | Independent end-opening booleans; legacy null choices are resolved once by the GUI |
| fuselageHoles | Top/Bottom/Left/Right closed-loop sketch layers with active wall/tool, selection, editing and pending points |
| fuselageThickening | Boolean initialized by complete-model generation or Thicken entry; retained across modes |
| fuselageStations | Vertical Side View sections with top/bottom curve anchors, optional profile slot, optional thicknessMm and selected index |
| fuselageProfiles | Stable cross-section sketch slots, active layer/tool, selection, editing state and pending points |
| fuselageOutline | Two sketch layers in Top/Side order, active layer/tool, selection, editing flag and pending points |
| airfoilSketches | Independent airfoil trace layers and their editing state; never interpreted as wing planform outlines |
| stations | Ordered LE/TE anchors, alignment, optional airfoil library index, selected station and optional pending first anchor |
| airfoils | Named imported profiles or closed trace snapshots, ordered boundaries, current choice and named unfinished draft |
| controlSurfaces | One two-control array per panel, selected panel, hinge/rectangle settings and pending drawing state |
| spars | One array per outline panel, each containing Top/Bottom/Mid spar records |
| lightening | Whole-wing enabled flag, physical wall/rib/setback lengths, crossmember count and entered dimension text |
| dihedralDegrees | One relative root angle in degrees per wing panel, ordered root to tip |
| ui | Workspace index, active toolbar label, viewport tab, 2D zoom/center, optional 3D camera and splitter sizes |

Sketch layers contain `[x,y]` points and curves referencing zero-based point
indices. Curve/tool values are 0 None (tool only), 1 Line, 2 fitted Spline,
and 3 Circle (fuselage profiles only).
Station anchors contain a layer index, curve index, displayed arc-length fraction
from 0 through 1, and current scene position. Alignment values are 0 Free,
1 Horizontal, 2 Vertical. Station airfoil indices refer to the ordered shared
library. Imported coordinates are normalized TE/LE/TE contours; traced airfoil
boundaries remain screen-oriented and are normalized during solid generation.

Reference units are 0 millimetres or 1 inches. Optional physical sizes and lengths
use JSON null. Each page stores `png` (base64 lossless PNG) and optional
`physicalMm: [width,height]`. The saved filename is provenance, not a required
external dependency. The original reference is not reread on Open.

UI workspace IDs are stable: 0 Reference, 1 Wing, 2 Fuselage, 3 Horiz Stab,
4 Vert Stab, 5 Assembly, 6 Export, 7 Weight and Balance, 8 Inspect. Visual toolbar
order ends with Inspect, Weight and Balance, Export; it does not change saved IDs.
Viewport is 0 2D or 1 3D.
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

View and editor-selection state is serialized in current projects, but is excluded
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
Version 2 introduced these fields (ADR-0009); current Save/Save As writes version 33.

Generated control surfaces use a fixed 1/16-inch (1.5875 mm) clearance at each
spanwise rectangle end, on the moving body only. This is a generation rule, not
an editable project field; version 2 rectangles and hinge settings remain
unchanged. After opening an existing project, explicitly selecting 3D regenerates its controls with this rule.

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
into Panel 1, with others disabled. Current Save/Save As writes version 33; older
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

Versions 1-4 preserve their shared library and stations. Former global controls
load into Panel 1, with other panels disabled. Legacy cross-panel stations are
retained for correction; each panel must have its own valid LE/TE pair before
completion/generation. No automatic duplication of controls or assignment to
new stations occurs during decoding. Versions 1-4 apps reject version-5 files.

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

Versions 1-7 initialize Lightening disabled. Current saves write version 33; older
apps reject it. No source file changes until Save. All project lifecycle paths
preserve these settings; New resets defaults. Text participates in dirty checking
but is omitted from the model fingerprint. The current Lightening toolbar mode
is captured by the existing UI tool field. See lightening.md and ADR-0017.

## Version 9: fuselage outline views

`fuselageOutline` uses the existing sketch schema and requires exactly two layers:
Top View (0) and Side View (1). `ui.fuselageView` is -1 when both view buttons are
off, 0 for Top, or 1 for Side. A selected view must match the sketch active layer.
Editing requires Fuselage/Outline, the 2D viewport and a selected view. Pending
points require an active drawing tool and editing. Open or empty loops remain
valid saved work; closure determines Profile Stations readiness, not file validity.

Idle selection, view and tool state is saved but excluded from dirty comparison.
Pending points retain their active layer/tool meaning in that comparison. Fuselage
inputs do not affect the Wing geometry fingerprint. Versions 1-8 initialize two
empty layers with both view buttons off; older applications reject version 9.
Original files are upgraded only on explicit Save. See ADR-0019.

## Version 10: fuselage profile stations

`fuselageStations` contains `lines` and `selected`. Each line has `top` and `bottom`
anchors using the existing layer/curve/parameter/position schema. Both anchors
must belong to Side View (layer 1), have matching X within 1e-7 scene units, and
have a positive height of at least 1e-7. Duplicate X locations within 1e-6 are
rejected. Alignment is implicitly Vertical; there is no Wing airfoil assignment
or pending first anchor. `selected` is -1 or a valid line index and is excluded
from dirty comparison. Invalid anchors or records reject Open transactionally.
Versions 1-9 receive an empty collection; earlier applications reject version 10.

## Version 11: station-linked profiles

`fuselageProfiles` uses the sketch schema. Each `fuselageStations.lines` record
adds `profile`, a nullable zero-based slot into these layers. Assigned slots must
exist and cannot be shared by multiple stations. Moving, deleting or reordering
other stations never changes a link. Delete Profile clears its slot and assignment;
unreferenced slots can remain in the document. Versions 1-10 load empty profiles
and unassigned stations. Generated solids, meshes, worker state and component
fingerprints remain transient. Save writes version 33; earlier versions remain readable.
Profile tool/selection/navigation state is excluded from dirty checks when no
points are pending. Pending sketches remain attached through Save/Open.

## Version 12: per-station fuselage walls

`fuselageThickening` is a required boolean. Each fuselage station adds
`thicknessMm`: null before initialization, otherwise a finite value from 0.001
to 10000 mm. An enabled thickening project requires a value at every station.
Thickness stays on the station when it moves and is removed with the station.
Profile deletion does not discard the station's thickness.
Versions 1-11 migrate to disabled thickening and unset values, initialized only
when Thicken is entered or a complete fuselage is generated. Existing files are upgraded only on Save. Before version 30, end closure
was inferred from outline/profile geometry. Version 30 adds stored end choices.

Fuselage thickness entry follows Reference units and accepts explicit mm/in.
`thicknessMm` remains normalized to millimetres; display-unit changes require no
format revision and do not rescale saved thicknesses.

## Version 13: Top/Side fuselage cuts

Versions 13–30 used two `fuselageCuts` layers: Top then Side. Version 31
uses four layers (Top, Bottom, Left, Right), migrating Side to Left.
It persists points, Line/Spline curves, shared endpoint indices, active view,
selected curve, tool, editing and pending points. Open and closed paths are
accepted without outline-loop validation. Editing is valid only in Fuselage/Cut
and 2D View; pending points require an active drawing tool and editing state.
Versions 1-12 load empty cut layers. New saves write version 33. Generated cut
bodies remain transient. Invalid indices, layer counts or draft states reject Open.

## Version 14: Servo Tray placement

`servoTray` contains `rectangle` (null or [x,y,width,height] in Side View scene
coordinates), `first` (null or a pending corner), and boolean `drawing`.
Rectangle width/height must be positive. A first corner requires drawing mode,
Fuselage/Servo Tray and the 2D viewport. Older versions initialize an empty tray.
The dimension-entry UI derives Width and Height through Side View scaling; no
separate thickness field is stored. Legacy freehand drafts are read but cleared
on restoration; new saves write first=null and drawing=false.
Generated tray/support solids and top-face outlines remain transient.

## Version 15: Formers

`formers` contains `rectangles`: up to 1000 [x,y,width,height] arrays in Side View
scene coordinates, with finite values and positive width/height. `thicknessMm`
is the next-former physical thickness, greater than zero through 10000 mm. Each rectangle
retains its own thickness through its width and Reference scale. Version 24 adds
per-former angles (see below). Positive-area rotated-mask overlaps with other formers or the tray are rejected. Touching is allowed.
Selection and generated shapes are transient. Versions 1-14 initialize no formers
and a 3 mm next thickness. The legacy Fuselage tool name Firewall maps to Formers.

Insert regeneration initializes missing station walls and enables `fuselageThickening`
before taking the model snapshot; the initialized values are retained by the next Save.


## Stabilizer outlines (version 16)

`horizontalStabilizerOutline` and `verticalStabilizerOutline` each store the standard
sketch object with exactly one layer: points, Line/Spline curves, pending points,
active layer (0), selected curve, tool and editing state. These are independent of
Wing and Fuselage sketches. Draft points require an active Line/Spline tool and
editing in the matching stabilizer workspace, Outline tool, and 2D viewport.
Invalid indices, extra/empty layers and inconsistent draft states are rejected
before replacing the current document. Incomplete or misaligned geometry is allowed
so work in progress can be saved and repaired.

Versions 1-15 load empty stabilizer outlines. For stabilizer UI state, old `Airfoils`
maps to `Airfoil`; removed `Airfoil Stations` and `Edit` selections map to `Outline`.
Only one airfoil type per stabilizer is supported; version 17 stores the selection
in `stabilizerAirfoils`.

Outline geometry and unfinished points affect document dirty state. Idle tools,
selection and navigation do not. Both sketches remap when the same reference changes
scale. Neither enters Wing or Fuselage generation fingerprints. New/Close clears both.


## Stabilizer airfoils (version 17)

`stabilizerAirfoils` has exactly two entries: horizontal, then vertical. Each is
null (use the bundled resources/naca009.dat) or an object containing `name` and
`coordinates` (normalized DAT `[x,y]` pairs). Imported coordinates are embedded;
original file paths are not required to reopen. Reject invalid array counts,
empty/multiline names, nonfinite coordinates and profiles with no thickness.
Versions 1-16 receive null defaults without visiting Airfoil. Generated stabilizer
workers, cancellation state, generated shapes and caches are transient. Changes to a selected
airfoil affect document dirty state and only its component's geometry fingerprint.

## Stabilizer leading-edge endpoints (version 18)

Each stabilizer sketch layer may contain `leadingEdge`, the integer index of its
chosen leading-edge root point. Omission means no selection, making the outline
incomplete for dependent tools and generation. Readers reject out-of-range indices
and points that are not open endpoints. Point moves retain the index; curve deletion
remaps it or clears it when the selected endpoint disappears. Selection mode and
hover state are not stored. The selection participates in document dirty state and
the component's generation fingerprint. Versions 1-17 load without a selection;
the user must choose it. Existing files are upgraded only when saved.

## Stabilizer hinge lines (version 19)

`horizontalStabilizerHinge` and `verticalStabilizerHinge` are single-layer sketch
records. They allow only Line curves, no leadingEdge, and a None/Line tool. A draft
is one pending point with active Line editing in the corresponding Hinge Line/2D
workspace. `stabilizerHingeCuts` is exactly two integers: 0 Tape, 1 Standard.
Invalid enums, non-line curves, invalid indices or inconsistent drafts are rejected.
Geometric separation validity is checked during generation so unfinished paths
remain editable. Versions 1-18 default to empty hinges and Tape; no file changes
until saving. Hinge geometry and style affect only their component's model cache.

## Stabilizer Cut Shapes (version 20)

`horizontalStabilizerCuts` and `verticalStabilizerCuts` are sketch records with
1-1000 layers, one loop per layer. Each layer holds Line/Spline curves and points;
leadingEdge is not allowed. `active` selects a shape and `selected` refers to a
segment within it. Editing/drafts are allowed only in that component's Cut/2D
workspace, and drafts require a drawing tool. Open/incomplete geometry is retained
for editing; generation checks closure. An empty placeholder layer represents no
Cut Shapes. Versions 1-19 load empty cuts. Selected layer/tool state is excluded
from dirty fingerprints unless needed for a pending draft. Geometry is included in
the stabilizer generation fingerprint. Cut-out material is removed, not retained.

## Assembly placement and cuts (version 21)

`assembly` contains boolean `positioned`, `offsets` (exactly three `[x,z]` pairs
in millimetres, ordered Wing, Horiz Stab, Vert Stab), and boolean `cuts`.
The optional `rotationDegrees` array contains exactly three finite angles in
the same order, each within [-180,180]. Positive is clockwise in Assembly's
standard side view about the component's root chord midpoint. Missing angles
default to zero, including existing version-28 files. Current writers include
the array; earlier readers may ignore it, so use a rotation-capable build to
retain these placements. Root pivots are derived from inputs, not serialized.
Each coordinate must be finite and within +/-10,000,000 mm. `cuts=true` requires
`positioned=true`. Versions 1-20 initialize false flags and zero offsets.
The first successful Assembly preparation chooses starting positions when
`positioned=false`. Opening a file always selects 2D; a saved Assembly workspace opens in Fuselage
Side View. Explicit entry into Assembly selects 3D and prepares the models.

Translations and cut intent affect document dirty state, but not individual
component generation fingerprints. Selected component is transient. Source edits
clear derived cut intent/caches while retaining translations. Solids and meshes are not serialized. Entering Assembly after Open
rebuilds components and reapplies requested cuts. Failed/cancelled cuts remain uncut.
Applications supporting only earlier formats reject current version 33. Original files change only on Save.

## Retired model caches (version 22) and input-only saving (version 23)

Version 22 embedded generated OCCT geometry and meshes in a `models` object.
Version 23 no longer writes that field. The current reader accepts versions 1–33
and ignores `models` entirely, including malformed or corrupt cache payloads.
It performs no base64 decoding, decompression, checksum validation or BREP reads
for obsolete caches. Normal design input validation and the 512 MiB file limit
still apply. The JSON itself must be valid.

All generated component, accessory, and original/cut Assembly geometry stays in
memory for the current session. Saving does not clear those caches. Opening starts
with empty caches and 2D selected; entering 3D or Assembly regenerates geometry
from saved inputs and reapplies requested Assembly cuts. Saving an older file
writes version 33 without its embedded models. Existing files remain unchanged
until Save. Applications supporting only earlier formats reject current version 33.

## Former rotation (version 24)

`formers.rotationDegrees` is an array with exactly one finite number per
`formers.rectangles` entry. Values range from -360 through 360 degrees. The
rectangle stores unrotated center/width/height; rotation is about its center in
Side View, negative counter-clockwise and positive clockwise. Width remains
normal thickness. Positive-area overlap validation uses the rotated masks for
former/former and former/tray pairs. Touching is allowed.

Versions 1–23 initialize former angles to zero. New saves write version 33;
older applications reject it rather than silently generating unrotated inserts.
Version-22 model caches remain ignored, and generated models are never written.

## Version 25

Adds `fuselageHoles`, a SketchState with exactly four layers in Top, Bottom,
Left, Right order. Each layer may contain multiple independent closed loops or
unfinished drafts. Active layer, selected curve, tool, editing and pending points
use the existing sketch representation. Versions 1-24 load four empty layers.
In version 25 the existing two-layer `fuselageCuts` remained unchanged; its connected paths became
managed individually by the shared Add/Delete controls.

## Version 26: Weight and Balance

`weightBalance` stores `densityKgM3` (XPS, default 25.63),
`plywoodDensityKgM3` (Aero Plywood, default 680), and `parts` (up to 1000).
Densities are finite, positive, from 0.001 through 10000 kg/m³. Each part stores
`name` (trimmed, unique ignoring case, 1–200 characters, single line), `widthMm`,
`heightMm`, `lengthMm`, `grams`, `centerMm` ([X,Z] in fuselage model millimeters),
and `ounces` (boolean entry-unit preference). Dimensions range from 0.001 through
10000 mm; weight ranges from 0.001 through 1000000 g. Position coordinates are finite within ±10000000 mm.
Y is implicitly zero; uniform mass acts at the rectangle center. All parts and
both densities affect dirty state, but not component-generation fingerprints.

Workspace 7 is Weight and Balance. Part selection/drag gestures, measured model
volumes and calculated totals are transient. Versions 1–25 initialize default
densities and no parts; original files change only on Save. These fields were
introduced in version 26; current saves use version 33.

The Reference UI now uses Wingspan for all manual project scaling. Legacy
`fuselageLengthMm` and `fuselageText` remain readable but do not control geometry.
Existing files therefore open without a format migration; their regenerated
fuselage/stabilizer sizes follow the shared wing scale (ADR-0040).

Undo/Redo history, highlighted orphan profiles, and model-only end registration
are transient. Recovery changes the existing station profile index; deleting an
orphan empties its stable sketch slot. Neither requires a format revision.

## Smoothed airfoil copies

Smoothed copies use the existing imported-profile representation with embedded
normalized coordinates. Originals and station assignments remain unchanged.
Preview strength is transient; no persistent field or version change is needed.

## Optional airplane statistics cache (introduced in format 29)

`airplaneStatistics` stores nullable finite numeric fields `wingspanMm`,
`wingAreaMm2`, `rootChordMm`, `aspectRatio`, `fuselageLengthMm`,
`horizontalAreaMm2`, `verticalAreaMm2`, `weightGrams`, `cgFromLeadingEdgeMm`,
and `wingLoadingGramsPerDm2`. Only CG may be negative. Missing fields mean
unavailable. Optional `balance` stores `sourceKey` (64 lowercase hexadecimal
SHA-256 characters), `leadingEdgeMm`, three `volumesMm3` and three `[x,z]`
`centersMm`, ordered foam, plywood, carbon fiber. Volumes are nonnegative.
No individual component geometry or breakdown is serialized. The reader validates
numbers and array sizes; the GUI recomputes outline statistics and discards mass
measurements whose source key no longer matches. Earlier projects omit this
cache; earlier format-29 readers can ignore it safely. See ADR-0047.
