# Airplane statistics

Every data panel except Assembly, Export and Weight and Balance has a compact footer showing available
Wingspan, Wing Area, Root Chord, Aspect Ratio, Fuselage Length, Horiz Stab Area,
Vert Stab Area, Weight, Wing Loading and CG from wing LE. Unknown values are omitted.
Airfoils and other expanding lists/tabs become shorter to make room. The footer
sits above bottom actions such as Smooth/Export on Airfoils.

Lengths use Reference units; areas use mm² or in². Weight uses grams or ounces.
Wing loading uses g/dm² or oz/ft². CG uses the placed wing root leading edge and
is positive aft, negative forward, matching Weight and Balance. Weight and Balance
has its own Wing Loading line, without the shared footer.
Assembly and Export defer mass integration entirely, including when entering
Export with no cached Weight and Balance measurement.

Sketch-only calculations require a physical Reference scale. Wingspan is the
entered full span in Specify Dimensions or the calibrated outline span in Use
Reference Image Scale. A complete single panel can establish its root from the
two open endpoints before airfoil stations exist. Multi-panel roots use the same
frame as generation. Root chord is measured between the root endpoints.
Wing area sums root/tip-closed panel outlines and doubles the result; horizontal
stabilizer area also doubles its half outline, while vertical stabilizer area
uses one outline. These are nominal unfolded planform areas, including controls,
before holes, interface cuts or dihedral projection. Aspect ratio is span²/area.
Fuselage length is the scaled Side View X extent. Outline edits, reference scale
and unit changes refresh these values without building any solids.

Weight, CG and wing loading become available after Assembly material measurements
are available. The same foam/plywood/carbon volumes and centroids, placed datum,
densities and added-part weights used by Weight and Balance supply the results.
Existing solids are integrated using the established statistics cache; no
regeneration is started solely to display statistics. Density and added-part
changes reuse these measurements. Wing loading is total mass divided by wing area.

The project saves the values plus aggregate material measurements and a SHA-256
key of the component generation inputs and Assembly placement/cut state. On open,
outline values are recalculated; saved mass measurements are used only when the
key matches. Geometry/placement changes clear stale weight/CG/loading until
current Assembly measurements exist. Density/part changes can update saved mass
statistics even before regenerating after reopen. The persistent summary cache
does not restore model solids or make Weight and Balance's full component table
available without a current Assembly. Derived cache updates are excluded from
edit history and the project-modified fingerprint, but included on Save.

Assembly has no statistics footer. While Assembly is active, title refreshes,
translations, rotations, and cut-state changes invalidate stale saved mass/CG/
loading without refreshing statistics or integrating solids. Measurements are
deferred until leaving Assembly for a panel that displays statistics, including
Weight and Balance. Its existing fingerprint checks ensure the next measurement
uses the current placement and geometry.
