# ADR-0010: Fixed control-surface end clearance
Status: Accepted
Date: 2026-09-14

## Context
Ailerons and flaps require a 1/16-inch gap on both non-hinge spanwise sides.
Hinge operations are already expensive.

## Decision
Keep the full rectangle opening in the fixed wing and extract the moving body
with both spanwise ends inset by 1.5875 mm. Convert this physical distance to
scene coordinates before constructing the extraction prism. Preserve the hinge
bevel algorithm and mirror the finished bodies. Reject insufficient span width.
An inset outside the wing has no effect on an already free end.
The clearance is a fixed generation rule; saved version-2 inputs need no new
field, and existing projects receive the gap when regenerated.

## Alternatives Considered
Two additional cutting tools would add unnecessary Boolean work. Enlarging the
fixed-wing opening would remove material outside the user-defined rectangle.
Adding an editable clearance setting is outside the requested scope.

## Consequences
The moving part is shortened, while the fixed wing and hinge contact heights
are preserved. Units and reference scale do not alter the physical clearance.
This supersedes the original zero end-clearance statement in ADR-0009.

## Validation
ControlSurfaceTests checks removed volume, material on both sides of each gap,
physical scale invariance, alternate scene axes, narrow-rectangle rejection,
and mirrored airfoil-wing bodies. Project and workflow tests verify restoration
and full model generation.
