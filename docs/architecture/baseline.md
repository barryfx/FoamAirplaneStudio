# Baseline and current application shell

Updated: 2026-09-25

The original baseline was copied read-only from DesignRC commit
8e8bcac9e9783d35915d76d296bc45282301c2f9. The original file hashes remain in
docs/baseline/source-manifest.json as provenance, not current-file checksums.
DesignRC is not modified by this project. OCCT is consumed from sibling
third_party. Existing requirements and GUI Design.txt remain authoritative.

## Current application

Holes provides closed-loop cuts through one selected fuselage wall, with four
wall choices and shared Add/select/Delete controls with Cut (fuselage-holes.md).

MainWindow owns a QWidget dataPanel with ReferencePanel, a 2D PlanViewport, and a 3D
OcctViewport. Primary selectors follow GUI Design.txt; secondary selectors change
with the component. ReferencePanel loads backgrounds and sets project units/dimensions (see reference.md).
WingOutlinePanel now supplies numbered sketch layers, line/spline tools and point
editing through the reusable SketchEditor (see sketch-editor.md and ADR-0002).
Airfoil Stations now provides curve-attached station placement and editing;
see airfoil-stations.md. AirfoilPanel adds a named imported/traced library and
per-station assignments (airfoils.md and ADR-0004). Dihedral supplies a Root Dihedral angle for each wing panel. All outer tips
use rounded geometry. WingSolidBuilder now generates the mirrored wing whenever ready inputs
and the Wing/3D view are active (wing-solids.md and ADR-0005).
Ailerons/Flaps adds selectable rectangles, hinge relief and end clearance
(control-surfaces.md). Spars adds surface grooves and mid-plane splits with
alignment tabs (spars.md). Lightening adds accessible hollow main-wing bays,
uniform crossmembers and protected alignment supports (lightening.md).
Generation runs as cancellable background jobs with bounded panel concurrency and mirrored display-mesh reuse (regeneration.md and ADR-0018). Fuselage supplies Top/Side closed-outline editing (fuselage-outlines.md). Fuselage Profile Stations places vertical Side View sections (fuselage-stations.md). Edit Profiles attaches cross-sections to stations and generates a separate cached fuselage solid (fuselage-profiles.md). Thicken supplies saved per-station walls and geometry-driven end closure (fuselage-thickness.md). Cut splits the body along persistent Top/Side paths (fuselage-cuts.md). Servo Tray adds a separate inner tray and integrated side ledges (servo-tray.md). Formers adds full/partial-height cavity-fitted inserts and 4 x 3 mm retaining rails on both inner sides (formers.md). The largest post-cut fuselage body splits into left/right halves with four mating alignment pins; other cut-outs stay whole (fuselage-cuts.md). Stabilizers provide Outline, Airfoil and independent solid generation. Stabilizer Cut supports closed material-removal shapes. Assembly provides positioning and reversible mating cuts (assembly.md); Export provides former STEP/DXF/STL and component STEP/STL from Assembly (export.md).

Edit provides Undo/Redo (Ctrl+Z/Ctrl+Y) and Copy/Paste; View camera/fit commands,
Help contains only About; the former Help entry was removed. About describes the
airplane design/manufacturing workflow, credits development using OpenAI Codex,
and retains copyright, GPL and library attribution without a local license path.
New remains. See editor-history.md. New clears
the views and returns to Reference after an unsaved-change prompt. The 3D viewport
also shows Fit View, Reset, Top, Bottom, Front, Back, Left, and Right buttons in a
bottom-centered toolbar. These share the View menu's QAction objects, including
their processing-lock enabled state. The toolbar is a native child of the OCCT
viewport so redraws do not cover it; it is hidden with the 3D tab and stays
centered when resized. New/Open/Close/
Save/Save As use `.foam` files (projects.md and ADR-0006). Wing modeling is
described in ADR-0005.

## Removed during UI cleanup

- WingPanelEditor and LengthInput, associated field data/defaults/JSON helpers,
  panel count, panel tabs, and field-driven statistics.
- Generate Wing, Generate Plan, and field-driven export actions.
- MainWindow's preview computation, parameter conversion, configured-joiner
  assembly, error correction, workers/cancellation, legacy generation alternatives,
  and joiner backend regression command.
- Flattened-wing technical-plan construction dependent on WingPanelData.
- Old GUI field/default tests and the old OCCT geometry regression contents.

## Retained reusable code

- Domain airfoil import/interpolation, wing/structural algorithms, and DXF/SVG export.
- OCCT geometry construction/Boolean helpers and STEP exporter.
- OCCT interactive viewport/camera controls.
- TechnicalDrawingDocument primitives, PlanViewport display/PDF support, and
  PartPdfExporter utilities, independent of the deleted wing editor.
- Domain and STEP export tests and fixtures.
- A GUI viewport regression and a skipped geometry-test shell for future work.

These utilities remain provisional; the new app does not invoke the old wing
generation sequence. The cleanup implements the user's requested removal without
selecting a replacement geometry architecture. Reference loading later added Qt PDF (ADR-0001).

## Historical cleanup validation

See docs/baseline/ui-cleanup-validation.md. Tests must not be reported as run for
this cleanup; the user explicitly requested build and application launch only.


Wing secondary actions now follow prerequisite-based availability; see wing-workflow.md.

Stabilizers use independent outline editors, one DAT airfoil (bundled NACA009 by default), and cancellable 3D workers with component caches. Version 17 introduced selected airfoils alongside the outlines from version 16. Version 18 persists the required leading-edge endpoint choice. See stabilizer-outlines.md and stabilizer-solids.md.

Hinge Line is implemented for both stabilizers, with connected straight segments
and Tape/Standard relief on the segment closest to vertical span for fins, or the longest segment for horizontal stabilizers (stabilizer-hinges.md, ADR-0030).
Matching horizontal halves join across the centerline, leaving one solid without
a hinge or separate fixed/elevator solids with one. Cut Shapes apply afterward
and may create additional pieces (stabilizer-cuts.md, ADR-0031). Versions 19 and
20 persist hinges and Cut Shapes respectively; current saves use version 29.
Only successfully published stabilizer results become cache entries. Assembly is enabled after complete component definitions; Export enables after successful current Assembly generation.

Assembly preparation and cuts use independent background jobs and preserve source
caches. Format 21 retains placements and cut intent; see assembly.md and ADR-0033.

The primary toolbar ends with Inspect, Weight and Balance, then Export. Inspect
provides normal 3D navigation, component visibility and persistent export names
(inspect.md). Weight and Balance supplies RC part placement, mass/CG and wing
loading from cached foam/plywood/carbon-fiber statistics (weight-and-balance.md).
Other panels show available dimensions and mass statistics in a shared footer
(airplane-statistics.md). The current persistence format is 29.

## Application icon

The Qt application/window icon uses the embedded
`resources/graphics/FoamAirplaneStudio.png` on all platforms. This is the only
file in the graphics directory. Unused legacy DesignRC icons and tip illustrations
have been removed. Windows executable resources use `resources/windows/app.rc`
and `FoamAirplaneStudio.ico`, derived from that PNG at 16, 24, 32, 48, 64, 128 and
256 pixels. When replacing the PNG, regenerate the ICO from the same image;
no Python or image-conversion dependency is required to build the application.

Startup displays the embedded `FoamAirplaneStudio.png` in a centered, aspect-preserving splash (up to 512 pixels). Main-window initialization occurs behind it. A precise, nonblocking timer keeps it visible for three seconds before revealing the maximized application; clicks do not dismiss it early. Slow initialization may extend this duration until the window is ready.
