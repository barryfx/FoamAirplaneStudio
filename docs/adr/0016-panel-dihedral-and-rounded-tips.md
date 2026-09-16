# ADR-0016: Per-panel root dihedral and rounded outer tips
Status: Accepted
Date: 2026-09-15

## Context
The user replaces selectable wing-tip treatments with per-panel Root Dihedral,
requires cumulative raised panel assembly and mating roots/joints, and requests
zero angles for older files. All generated wings must use rounded outer tips.

## Decision
Replace the fourth Wing tool with Dihedral and store one relative degree angle
per panel. The first angle is relative to horizontal; subsequent angles increment
the preceding orientation. Keep the existing project/camera invalidation rules.
Version 7 adds dihedralDegrees and ui.dihedralPanel, removes wingTip, and reads
versions 1-6 with zero angles and a mapped tool selection. No new dependency.

Construct nonzero-angle panel sections with local span displacement k(y)*z.
At the first root k=tan(first angle); at later roots k=tan(increment/2), and at
internal tips k=-tan(next increment/2). Linearly interpolate k between ends.
This yields angle-bisector mating planes after rigid rotation. Project mitered
root/end chords onto equal-span caps. Build controls and spars in this panel
frame, extending full-length spar cutters through the mitered caps. Then rotate
all bodies by cumulative angle, place each root at the preceding generated tip,
and mirror the completed half across Y=0. Matching chord/profile definitions
produce matching full joint faces; independently different profiles retain their
intentional step rather than being silently replaced. The all-zero path retains
flat trace placement for migrated designs.

Always round the outermost tip's local thickness toward its midpoint with a
circular profile, radius half thickness, following the traced outboard contour.
Sixteen extra cosine-spaced sections resolve the closure region. A 0.1% retained
thickness floor and 0.00001 mm TE half-gap avoid degenerate OCCT closure topology.
The internal near-terminal cutoff remains in effect and excludes the outer tip.

## Alternatives Considered
Rotating square-ended bodies alone leaves wedge gaps/overlap at roots and joints.
Cutting only after rotation shortens one side of the sections and can truncate
spar cuts. Absolute angles at every panel do not express the requested root
increment from the preceding panel. Dropping old-file support would lose projects.

## Consequences
Projected span shrinks as dihedral increases; input scale describes unfolded span.
Angle fields permit -80..80 degrees; generation requires cumulative orientation
strictly between -85 and 85 degrees. Very short/thick panels or incompatible
features can still fail solid validation with an explicit diagnostic. Old tip
choices migrate to rounded geometry; old angles are zero. Original files remain
unchanged until Save. Earlier tip-choice decisions are superseded.

## Validation
Tests cover panel ownership/defaults, view-only navigation, save/open/migration,
root mirror mating, opposite sides of joint planes, cumulative tip elevation,
rounded-tip thickness, invalid angles, camera preservation and feature cuts.
Execution and visual evidence are recorded in docs/baseline/dihedral-validation.md.
