# Fuselage stiffeners

Fuselage / Stiffeners enables after all station profiles are closed. Count is
per side (0–16), mirrored left/right; zero disables the feature. Strip uses
vertical Width and inward Height; Round uses Diameter only. Bare lengths use
Reference units; explicit mm/in is accepted. Start/Stop are percentages of total
fuselage length from the nose, with Start < Stop. Defaults are 50–95%, a 3 × 1 mm
strip or 3 mm rod. Saved inactive dimensions survive shape changes.

At each longitudinal section, one route uses local mid-height; multiple routes
use equal fractions of local height. Outer wall sections determine the lateral
position. Sixty-five samples check spacing, actual solid material and nominal
wall thickness; changes in successive tangent direction above 15 degrees reject
the range. Collinear samples are removed, then cubic lofts form smooth tools.
This is a sampled fit check, not a carbon bend-radius guarantee. Users must move
the range onto a smooth boom section if generation rejects a sharp transition.

Strip tools extend outward to open the surface; the carbon stock is the entered
rectangle. Round tools are centered on the skin, making nominal half-depth
grooves like wing surface rods. Each mirrored full rod/strip contributes volume
and centroid to Carbon Fiber in Weight and Balance. Stock remains in the
fuselage frame during Assembly wing placement and is not an exported part.

Grooves are cut after hollowing, inserts and mirroring, before holes, cuts and
alignment finishing. Later holes/cuts can interrupt foam while stock remains
continuous across the specified range. Failed/cancelled operations publish no
partial stock. Validity and solid-count checks reject disconnected results.

Settings participate in undo/redo, dirty state, project format 33 and the
fuselage fingerprint. Fuselage, Assembly and mass caches invalidate; unrelated
wing caches remain reusable. Older projects load count zero. See ADR-0052.
