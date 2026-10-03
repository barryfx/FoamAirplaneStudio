# Ailerons and flaps

Wing / Ailerons/Flaps presents Add Ailerons and Add Flaps, initially unchecked.
Each enabled section shows the rectangle instruction, Tape Hinge Cut (default),
Standard Hinge Cut, and Draw Rectangle. Checking a box activates rectangle drawing
in 2D. Click two opposite corners or drag between them. Completion ends drawing;
Draw Rectangle stays highlighted for the active drawing target, including a
restored drawing session. Completing a rectangle, Escape, unchecking the control,
or leaving the mode clears the highlight. Clicking the lit button cancels drawing;
clicking it again starts a replacement. Escape or leaving the mode cancels the pending
rectangle without removing the last committed rectangle. Only the active target
can be drawn; both enabled committed rectangles remain visible in every 2D mode.
Unchecking hides that section's details/rectangle and removes its cut from 3D.
The last rectangle and hinge choice are retained for re-enabling.

Place the rectangle over the trailing portion of the right wing, extending its
outer edge past the trailing edge. The LE-facing spanwise edge is the hinge.
Ailerons are magenta; flaps are orange, both with a dark border for contrast.
Do not overlap their rectangles. Outline/station editing is locked in this mode.

The fixed wing is separated from each selected part before removing hinge relief
from the control surface only. Tape hinge: contact at the upper wing surface,
45-degree gap below. Standard hinge: contact at the middle of local wing thickness,
equal 45-degree gaps above/below. Geometry is validated and the resulting separate
bodies are mirrored. Invalid placements report the reason instead of retaining a
stale model. See ADR-0009 for the sampled-height construction and limits.

ControlSurfaceState is shared by editor, project snapshot and geometry inputs.
ControlSurfacePanel owns widgets; PlanViewport owns the reusable rectangle editor.
Reference scale changes remap rectangle corners with other sketches. New and Close
reset the controls; Open restores them; Save/Save As preserve settings, enabled
flags, rectangles and an unfinished first corner. These fields were introduced in
version 2; current saves use version 32. Version-1 projects open with both controls disabled. Model updates follow the existing
data-change detection and retain the 3D camera.

The hinge Boolean operations execute once each. In OCCT 8, constructing Cut or
Common with two shapes already calls Build; a second explicit Build repeats
intersection and result construction. The optimized path retains the same input
shapes, 33 hinge samples, tolerances, body validation and before-mirroring order.
The manual control_surface_benchmark target measures fresh identical BREP inputs
and checks BREP identity (allowing only tighter vertex tolerances), with
symmetric-difference fallback for other differences; see
../baseline/hinge-performance.md for measurements and reproduction.

Each non-hinge spanwise end has a fixed 1/16-inch (1.5875 mm) clearance.
The fixed wing opening follows the original rectangle; the moving body is
extracted using a rectangle inset at both spanwise ends by 1.5875 mm divided
by the scene-to-mm scale. This removes material only from the control surface,
preserves the hinge profile, and adds no Boolean operations. Both ends are
trimmed before mirroring. An end outside the wing already has free space; its
inset plane only removes material if it intersects the wing. Rectangles narrower
than the two clearances are rejected. The rule applies equally to ailerons and
flaps and does not depend on project display units.

When Ailerons/Flaps is active and drawing is off, click inside an enabled
rectangle or within seven viewport pixels of its border to select it. The
selected rectangle has a thick cyan border and stronger fill. Only one rectangle
is selected; the topmost wins if rectangles overlap. Clicking empty space or
pressing Escape clears selection. Delete removes only the selected rectangle,
retaining its enabled flag and hinge style, and invalidates the generated model.
Starting drawing, disabling that control, restoring a project, or leaving this
mode clears selection. Selection is transient editor state, does not dirty the
project, and is not serialized. Deleted rectangles are saved as null using the
rectangle field in panel records (introduced in version 21). The panel describes drawing and selection controls.

Leaving Ailerons/Flaps mode or selecting 3D ends rectangle drawing and clears
any checked control without a committed rectangle. Checkbox widgets and saved
control flags update together. A pending first corner is discarded; a previously
committed rectangle and its hinge choice are retained. Deletion itself keeps the
checkbox checked until this transition so the user can draw a replacement.


## Overlap validation
Enabled aileron and flap rectangles cannot share positive area (touching edges
are allowed). An overlapping redraw is rejected in the 2D editor, keeps the
previous committed rectangle, and displays an inline explanation. The drawing
tool remains active for a replacement. Disabled rectangles do not participate.
Restored overlapping projects remain editable and display the conflict in the
panel; wing generation rejects them before lofting. The low-level cut function
also validates overlap. Main-wing panel and Mid spar splits never split the
separate aileron/flap bodies.

## Panel control settings
Numbered tabs hold independent Add Ailerons/Add Flaps flags, rectangles and hinge
choices. Drawing, selection and deletion affect only the selected panel. All
committed rectangles remain visible. Overlap checks include enabled rectangles
on every panel. Changing tabs cancels unfinished drawing; leaving the mode or
entering 3D clears missing-rectangle flags on every panel. Cuts apply only to the
owner panel; spar splits never split its control bodies. Version 5 introduced storage for all
panel settings. Older global settings migrate to Panel 1 only (ADR-0013).
