# Fuselage stiffeners

Fuselage / Stiffeners enables after all station profiles are closed. Count is
per side (0–3), mirrored left/right; zero disables the feature. Strip uses
vertical Width and inward Height; Round uses Diameter only. Bare lengths use
millimetres, independent of Reference units. Explicit inches remain displayed
in inches for that field until replaced with a bare/mm entry. Project unit
changes do not convert the fields. Display preferences are local to the panel;
restoring a project or undo snapshot defaults the display to mm. Start/Stop are percentages of total
fuselage length from the nose, with Start < Stop. Defaults are 20–90%, a 3 × 1 mm
strip or 3 mm rod. Saved inactive dimensions survive shape changes.

At each longitudinal section, one route uses local mid-height; multiple routes
use equal fractions of local height. Outer wall sections determine the lateral
position. Sixty-five samples check spacing, actual solid material and nominal
wall thickness; changes in successive tangent direction above 15 degrees reject
the range. Collinear samples are removed, then cubic lofts form smooth tools.
Clearance checking loads one OCCT solid classifier per body solid and reuses it
for every probe in that generation call. The body is unchanged during checking;
mutable classifiers are local to the call and never shared between workers.
Sampling and tolerances are unchanged. See the classifier benchmark evidence in
`../baseline/stiffener-classifier-reuse.md`.
This is a sampled fit check, not a carbon bend-radius guarantee. Users must move
the range onto a smooth boom section if generation rejects a sharp transition.

Strip tools extend outward to open the surface; the carbon stock is the entered
rectangle. Round tools are centered on the skin, making nominal half-depth
grooves like wing surface rods. Each mirrored full rod/strip contributes volume
and centroid to Carbon Fiber in Weight and Balance. Stock remains in the
fuselage frame during Assembly wing placement and is not an exported part.

Grooves are cut into the right half after hollowing and inserts, before
mirroring. The grooved half is reflected into the separate left half; stock
volumes are copied and centroids reflected without cutting the left side again.
The full-construction comparison path still cuts both sides for regression parity.
The groove Boolean uses OCCT parallel processing by default; the explicit
ProcessingControl serial override remains available for parity tests.
Holes, cuts and alignment finishing follow mirroring. Later holes/cuts can interrupt foam while stock remains
continuous across the specified range. Failed/cancelled operations publish no
partial stock. Validity and solid-count checks reject disconnected results.

Settings participate in undo/redo, dirty state, project format 33 and the
fuselage fingerprint. Fuselage, Assembly and mass caches invalidate; unrelated
wing caches remain reusable. Older projects load count zero. See ADR-0052.

Earlier format-33 projects with counts 4–16 load capped at three per side. Saved
Start/Stop percentages remain unchanged; 20–90% applies to new/default settings.
