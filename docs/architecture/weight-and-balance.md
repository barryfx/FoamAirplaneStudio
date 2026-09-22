# Weight and Balance

Weight and Balance follows Export and precedes Inspect on the primary toolbar. It enables once Reference has
a physical scale and the fuselage Side View is closed. In Specify Dimensions,
the wing outline and stations establish the shared Wingspan calibration first.
It is a 2D-only workspace
with instructions, Add Part, a named-part selector, Edit/Delete, density fields,
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
Default densities are 32.5 kg/m³ XPS and 680 kg/m³ birch aircraft plywood; users
can change both to match their actual stock. See ADR-0039 for sources.

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
the application's busy cursor before measuring foam and plywood. It repaints the
status synchronously without dispatching user input, and restores the cursor on
success or failure. The cache is transient and is never saved to the project.
Part shapes are mass envelopes only, with uniform mass at their centers; they
do not cut foam. Add spars, covering and other unmodeled weights as parts.
