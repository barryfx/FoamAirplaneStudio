# Weight and Balance

Weight and Balance follows Inspect and precedes Export on the primary toolbar. It enables once Reference has
a physical scale and the fuselage Side View is closed. In Specify Dimensions,
the wing outline and stations establish the shared Wingspan calibration first.
It is a 2D-only workspace
with instructions, Add Part, a named-part selector, Edit/Delete, a Material Densities button,
and totals at the bottom of the data panel and window. Parts appear only here.
Entering frames the Side View and placed parts; wheel zoom and scrolling remain available.

Add/Edit opens a modal dialog for a unique nonempty name, decimal Width, Height,
Length in Reference units, and weight in grams or ounces. Changing weight units
converts the value. Cancel preserves the existing record. New parts start at the
Side View bounding-box center. Width extends into the screen; Length and Height
size the displayed rectangle. Drag a rectangle to move its mass center. Select
from the list to access an overlapping part; the selected rectangle takes priority
when dragging. Delete removes the selected part; Escape clears selection.

Positions use the fuselage nose registration and physical X/Z coordinates.
Changing Reference display units or remapping the reference does not change
physical part dimensions, positions or mass. Idle selection is not project data.
New/Close reset parts and densities. Format 26 preserves all entered data.

The current Assembly snapshot supplies foam body volumes and centers, respecting
placement and optional interface cuts. Fuselage part records are not counted
again. Formers and the servo tray are separately integrated as Aero Plywood.
Enabled wing spars are automatically included as Carbon Fiber: solid surface
rods/strips and hollow mid tubes. Their cached material properties follow wing
placement and remain intact when Assembly cuts foam interfaces.
The component breakdown lists each foam component, plywood insert and carbon spar with
solid material volume in cm³ and weight in grams and ounces. Foam, plywood and Carbon Fiber
subtotals use the same measurements as the overall balance calculation. Missing
foam components are marked "not present". Entered parts show their supplied
weights and no material volume; their drawing boxes are not measured foam.
Component volumes share the existing statistics cache, so density edits update
all row weights without repeating CAD integration. Stale Assembly data clears
the geometry rows. The breakdown is derived and is not saved in the project.
Default densities are 25.63 kg/m³ XPS, 680 kg/m³ birch aircraft plywood and
1540 kg/m³ Carbon Fiber composite; users can change all three to match their
actual stock. The Material Densities dialog contains Foam, Aero Plywood, Carbon
Fiber and Resin (1500 kg/m³, or 1.5 g/cm³, by default). OK applies all values; Cancel changes
none. The Carbon Fiber field follows Aero Plywood density. See ADR-0039
and ADR-0046 for sources. CF density applies to material only, excluding bores.

For volume V in mm³ and density rho in kg/m³, mass in grams is V × rho × 10⁻⁶.
The total center is sum(mass × center) / sum(mass), including each entered part.
The longitudinal result subtracts the placed wing root leading-edge X, then
converts to mm or inches. Positive is aft, negative forward. Both g and oz totals
are displayed. No aerodynamic target CG or stability recommendation is inferred.

An absent/stale Assembly displays only the entered parts subtotal and explicitly
unavailable total/center. Generate the current Assembly to calculate the whole
model. Opening a project does not regenerate or trust old derived totals.
Part movement, resizing, deletion and density editing never regenerate geometry.
Solid volume and centroid statistics stay cached across workspace changes. Cache
validation uses the existing component-generation fingerprint, Assembly offsets
and cut state, project epoch, and source solid identities. Geometry regeneration,
placement changes and cut replacement therefore cannot reuse stale statistics;
part and density edits reuse volumes and recalculate only weighted totals.
On a cache miss, the standard processing scope displays a calculation status and
the application's busy cursor before measuring foam, plywood and carbon spars. It repaints the
status synchronously without dispatching user input, and restores the cursor on
success or failure. The per-component cache is transient; aggregate measurements may be saved separately for the shared statistics summary.
Part shapes are mass envelopes only, with uniform mass at their centers; they
do not cut foam. Use the component Fiberglass tabs for cloth and resin covering;
see [Fiberglass estimates](fiberglass.md). Each patch adds cloth and resin rows,
covered area, and contributions to total mass and CG. Add other unmodeled weights
as parts; do not add automatically counted spars or fiberglass a second time.

Wing Loading is also displayed when total mass and nominal full wing area are
available, in g/dm² for metric Reference units or oz/ft² for inches. The shared
statistics footer is intentionally omitted here. Other panels display dashes for weight, loading and CG and do not measure mass; see airplane-statistics.md.

Assembly movement and rotation do not trigger mass integration. The Assembly
panel omits the shared statistics footer, and stale saved totals are cleared
without measuring solids there. Enter Weight and Balance to refresh totals for the current placement.

The user-facing results and bottom status summary label the balance point Center of Gravity.

A small CG symbol overlays the Side View only in Weight and Balance when the
current model mass, wing datum and root airfoil are available. Its X coordinate
matches the displayed LE-relative CG. Its visual height is 20% of the root
airfoil's total vertical thickness above its lowest point, scaled by root chord
and following the wing's Assembly translation and rotation. This height is a
wing reference for the marker, not a claim about the actual vertical CG.
The bundled `graphics/CG Symbol.png` draws at 28 logical pixels, independent of
zoom. Part/density edits update its position using cached mass properties;
stale model data hides it without generating geometry.

Volume integration runs in bounded workers across independent components and
plywood inserts; fiberglass area integration runs across individual patches.
Workers own their accumulators and covering meshes; deterministic reductions
preserve component order and mass/CG results. Both cloth and resin entries show
the same covered surface area; the resin row also retains its material volume.

Weight and Balance uses up to 90% of reported logical hardware threads, rounded
down with a minimum of one, limited by the number of uncached tasks. Other
generation schedulers are unchanged. Nested covering meshing remains serial.

Individual component/insert volumes and patch areas/centroids have transient
caches beneath the aggregate cache. Editing one boundary measures only that
patch; unchanged foam and other patches are reused. Material values reweight
existing measurements. Topology, placement or relevant projection changes
invalidate only dependent entries. Keys compare immutable source topology,
orientation and numeric transforms; patch keys also include their geometry,
wrap/surface, scale and relevant outline inputs. Renaming/reordering/deleting
patches preserves reuse for surviving geometry. Successful queries replace the
retained entry set, so obsolete shapes do not accumulate. Failed measurements
leave the previous valid cache intact. New/Open/Close clears these caches; they
are not saved as portable CAD measurements. See ADR-0051.
