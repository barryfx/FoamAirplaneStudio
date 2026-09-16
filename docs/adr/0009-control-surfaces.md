# ADR-0009: Rectangle-defined control surfaces and hinge relief
Status: Accepted
Date: 2026-09-14

## Context
Wing/Ailerons-Flaps needs separate movable bodies, tape or standard hinge cuts,
reusable rectangle drawing and complete project restoration.

## Decision
Store one independently enabled rectangle and hinge choice for ailerons and one
for flaps. Rectangles use reference-scene axes and coordinates. The spanwise edge
facing the leading edge is the hinge; the opposite edge must overhang the trailing
edge. Choose the hinge side using the root-derived chord direction. Overlapping
rectangles, rectangles that do not intersect the hinge inside the wing, or cuts
that leave disconnected/empty parts are rejected with a status message.

ControlSurfaceEditor owns rectangle data and drawing interaction, independently
of wing outline curves. It supports two opposite-corner clicks or a drag. Checking
a box begins drawing; Draw Rectangle permits replacement after completion or
Escape. Unchecking hides the instructions and rectangle and removes the generated
cut, but retains its settings for reuse. Escape cancels only an unfinished rectangle;
a previously committed one is retained. Wing outlines and stations remain locked.
Magenta identifies ailerons and orange identifies flaps in all 2D modes.

Extrude each rectangle through the wing in Z. OCCT Common extracts the control
surface and Cut leaves the fixed wing. Cut relief only from the extracted body.
Tape hinges use d >= upperHeight - z: top-surface contact and a 45-degree lower gap.
Standard hinges use d >= abs(z - middleHeight): center-thickness contact with
45-degree upper and lower gaps. d is perpendicular distance from the hinge into
the control surface. Local hinge heights come from vertical intersections with
the original wing. Sample at 33 positions along the hinge and linearly loft the
cutting wedges; redundant linear samples are removed within 1e-6 mm. Continue
nearest heights over rectangle ends outside the wing. The result contains the
fixed body followed by aileron and flap bodies; mirror the complete collection.

Write project version 2 with a typed controlSurfaces record, including pending
rectangle drawing state. Continue reading version 1 with both controls disabled.
Older applications reject version 2 rather than silently losing these cuts.
The data/geometry fingerprints include control settings; idle drawing selection
remains view state, while a pending first corner is unsaved data.

## Alternatives Considered
Treating rectangles as ordinary wing curves loses their meaning and edit locking.
A single Boolean cut removes the control material without preserving its body.
Beveling the fixed wing conflicts with the requested control-side hinge relief.
A new optional field under version 1 would be silently dropped by older writers.

## Consequences
No dependency is added. Both halves have separate control bodies and are regenerated
from saved inputs. End clearance now follows ADR-0010; no hardware is modeled. Hinge height between samples is an approximation
for varying airfoils/curved wings; constant-height hinges are exact. Scene-aligned
rectangles do not define arbitrary swept hinge curves. An enabled control with no
committed rectangle has no geometry effect yet. Disabled rectangles are retained.

## Validation
ControlSurfaceTests verifies exact contact heights, 45-degree gaps and removed
volumes on constant-thickness bodies, two independent controls, invalid rectangles,
and separate mirrored NACA-wing solids. Editor tests exercise clicks, drag,
Escape, radio exclusivity, disabling, remapping and reset. Project tests cover
version-1 Open, version-2 Save As, pending corners and enabled/disabled rectangles.
Full workflow testing generates both cuts in the 3D viewport and retains camera.
