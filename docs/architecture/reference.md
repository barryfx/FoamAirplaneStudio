# Reference workflow

Reference shows Load Image, the loaded path, and mutually exclusive scale choices.
PNG, JPG/JPEG, and PDF are supported. All PDF pages load in document order, stacked vertically.
Load failures/cancellation preserve the previous reference. Selecting Reference
brings the 2D viewport forward. Backgrounds stay available when changing workspaces.
Load Image remembers the last accepted file across New and app restarts, and
preselects it next time. Cancel does not change this history. See file-dialogs.md
for the common file/directory chooser policy.

## Workspace prerequisites

A new project enables only Reference in the workspace toolbar. Wing becomes
available when Specify Dimensions has both positive, finite Wingspan and Fuselage
Length values (with or without an image), or Reference is to scale is selected
with a loaded image and valid physical dimensions for every page. Loading an
image alone does not unlock Wing. Hidden manual values do not override an invalid
actual-scale selection. Fuselage additionally requires completed Wing airfoil assignments; Top/Side outline, station and profile editing plus solid generation are implemented (fuselage-profiles.md). Assembly and Export remain disabled.

ReferenceWorkflow applies this rule at startup and on reference changes. Clearing
or invalidating a required dimension disables Wing again; New resets the gate.
If prerequisites are invalidated while another workspace is active, the app
returns to Reference without removing existing sketches. The Wing secondary
toolbar still follows its own outline/station/airfoil prerequisites.

Validation (Windows Debug, 2026-09-13): reference_tests and wing_workflow_tests
passed (2/2, 0.41 seconds). Coverage includes the initial toolbar, one versus both
manual dimensions, unit changes, invalid/cleared values, New, image-only input,
scaled PNG/PDF, missing per-page scale metadata and unchanged downstream gating.
Debug was rebuilt and launched. Linux/macOS remain untested.

Reference is to scale uses embedded physical resolution or PDF page size, sets the
project display units from the source metadata convention, and hides manual fields.
Raster files without explicit physical-size metadata require Specify Dimensions.
PNG physical size uses pHYs meters (displayed in mm); JPEG supports EXIF resolution
and JFIF inch/cm density. PDF point units display as inches. These units describe
file geometry, not text or dimension labels printed on the drawing.

Specify Dimensions shows Project Units (millimeters/inches), Wingspan, and Fuselage
Length. Values accept positive decimals with optional mm/in suffixes (also inch,
inches, millimeters, millimetres, or double quote). Bare numbers use Project Units; empty or invalid entries have no committed
dimension. Unit changes preserve entered values and physical lengths, appending
the previous unit suffix to valid bare values before changing the project default.
Explicit-unit entries keep their original numeric text and suffix. ProjectReference exposes these
values to component tools; state is per project/window and survives toolbar
changes. New clears the reference and dimensions and resets units to millimeters.

Manual Wingspan now calibrates the traced mirrored wing, not the entire image
rectangle (wing-solids.md). A plan can contain margins and multiple views.
Manual Fuselage Length calibrates the Top/Side fuselage outlines; stabilizers
use that length divided by the Side View extent and assume the same drawing
scale (fuselage-profiles.md and stabilizer-solids.md). `.foam` files embed reference
pages and their scale metadata (projects.md). Actual-scale backgrounds use millimeter
scene coordinates; uncalibrated previews use pixels.

Qt PDF is required in addition to Qt Widgets. Install a Qt PDF add-on matching
the Qt SDK; Windows deployment uses windeployqt. See ADR-0001 for dependency and
unit decisions.

Validation: Windows Debug built and launched; a loaded PDF was visually inspected
in the running app. The focused reference and viewport test suites now pass on Windows Debug; see
docs/baseline/reference-test-results.md. Linux/macOS remain unvalidated.

## Width fitting and scrolling

Loading, resizing, and Fit View fit the full stack width to the viewport. The
vertical scrollbar appears when required. Wheel/trackpad gestures zoom; scrolling
requires explicit scrollbar interaction. Horizontal scrolling becomes available when zoomed in. Resizing and Fit View refit the width.
Pages preserve their aspect ratio and relative physical sizes; for mixed page
sizes, the widest page determines the stack width. Each page is a separate image
item, so no giant stitched bitmap is allocated. Aggregate PDF rasterization uses
a common resolution capped at 32 million pixels and 4096 pixels per page side.
Loading fails atomically if any page cannot be rendered.



Cursor-centered zoom uses the wheel event position and preserves the scene point
under it after scrollbar layout settles. The view may include empty margins to
keep that point fixed when zooming out past an image edge. These margins do not
change reference coordinates or content bounds; Fit View still fits image width.
Saved zoom/center restoration permits the same margins.
