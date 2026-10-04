# Fuselage stiffeners

Fuselage / Stiffeners enables after all station profiles are closed. Count is
per side (0–3), mirrored left/right; zero disables the feature. Strip uses
vertical Width and inward Height; Round uses Diameter only. Bare lengths use
millimetres, independent of Reference units. Explicit inches remain displayed
in inches for that field until replaced with a bare/mm entry. Project unit
changes do not convert the fields. Display units are saved per dimension and survive project/undo restoration. Start/Stop are percentages of total
fuselage length from the nose, with Start < Stop. Defaults are 20–90%, a 3 × 1 mm
strip or 3 mm rod. Saved inactive dimensions survive shape changes.

Each groove is straight in Side View: at Start and Stop, one stiffener uses
50% of local height, two use 1/3 and 2/3, and three use 1/4, 1/2 and 3/4.
The matching endpoint heights define a line in X/Z. Intermediate fuselage
midpoints do not alter that line. Lateral Y is taken from the skin at the line's
height, so inward groove depth remains constant; the route can curve in Top View.
This is deliberately not a completely straight 3D rod axis. See ADR-0056.

Strip depth is its entered Height; round centerlines lie on the skin, giving a
nominal half-diameter depth. Width/diameter sections remain in Y/Z planes. Ruled
lofts form the cutter and stock from the same route, preventing smooth-loft
overshoot. The initial 65 samples are supplemented by wall-section positions.
Quarter/midpoint checks subdivide lateral spans when the interpolation error
exceeds the lesser of 0.01 mm and 1% of nominal groove depth; excessive required
refinement produces a diagnostic. Collinear samples are removed. This bounds the
sampled skin approximation rather than claiming mathematically exact continuous
surface distance. Depth is inward along Y, not normal to a sloped skin surface.

Clearance probes follow this actual side-view line and reject locations outside
the wall or through openings/cavities, with percentage diagnostics. Endpoint
spacing, nominal wall-depth and sampled lateral bend checks remain. The body is
immutable during checking; classifiers are local and reused rather than shared
between workers. The historical classifier benchmark is in
`../baseline/stiffener-classifier-reuse.md`; sample counts have since changed.

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

Failure diagnostics distinguish a failed OCCT Boolean, invalid resulting topology,
and an unexpected solid count. Extra solids are reported with volume and their
longitudinal span as percentages from the nose. Invalid faces report their span
when identifiable. These are affected geometry extents, not an asserted exact
break point. Kernel failures without localizable geometry report the requested
range and explicitly say the exact position is unavailable. Sampled clearance,
wall-depth, spacing and abrupt-bend failures identify the stiffener and sampled
percentage. Failed cuts still publish neither geometry nor carbon stock.
