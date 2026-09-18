# Baseline and current application shell

Updated: 2026-09-18

The original baseline was copied read-only from DesignRC commit
8e8bcac9e9783d35915d76d296bc45282301c2f9. The original file hashes remain in
docs/baseline/source-manifest.json as provenance, not current-file checksums.
DesignRC is not modified by this project. OCCT is consumed from sibling
third_party. Existing requirements and GUI Design.txt remain authoritative.

## Current application

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
Generation runs as cancellable background jobs with bounded panel concurrency and mirrored display-mesh reuse (regeneration.md and ADR-0018). Fuselage supplies Top/Side closed-outline editing (fuselage-outlines.md). Fuselage Profile Stations places vertical Side View sections (fuselage-stations.md). Edit Profiles attaches cross-sections to stations and generates a separate cached fuselage solid (fuselage-profiles.md). Thicken supplies saved per-station walls and geometry-driven end closure (fuselage-thickness.md). Cut splits the body along persistent Top/Side paths (fuselage-cuts.md). Servo Tray adds a separate inner tray and integrated side ledges (servo-tray.md). Formers adds full/partial-height cavity-fitted inserts and 4 x 3 mm retaining rails on both inner sides (formers.md). The largest post-cut fuselage body splits into left/right halves; other cut-outs stay whole (fuselage-cuts.md). Stabilizers provide Outline, Airfoil and independent solid generation. Stabilizer Hinge Line/Cut, Assembly and Export remain under development.

View menu camera/fit commands, Copy/Paste, Help/About, and New remain. New clears
the views and returns to Reference after an unsaved-change prompt. New/Open/Close/
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

## Validation

See docs/baseline/ui-cleanup-validation.md. Tests must not be reported as run for
this cleanup; the user explicitly requested build and application launch only.


Wing secondary actions now follow prerequisite-based availability; see wing-workflow.md.

Stabilizers use independent outline editors, one DAT airfoil (bundled NACA009 by default), and cancellable 3D workers with component caches. Version 17 introduced selected airfoils alongside the outlines from version 16. Version 18 persists the required leading-edge endpoint choice. See stabilizer-outlines.md and stabilizer-solids.md.
