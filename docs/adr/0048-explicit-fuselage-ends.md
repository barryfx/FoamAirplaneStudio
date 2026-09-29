# ADR-0048: Explicit fuselage end walls
Status: Accepted
Date: 2026-09-25

## Context
Inferring open ends from vertical profile positions misclassifies deliberate
slanted noses. A former can lie inside the outer drawing but outside the cavity.
The user requested independent explicit nose and tail choices.

## Decision
Store optional end booleans in format 30. New projects use Closed; legacy null
values preserve inference for headless tools and are resolved once by the GUI.
Include choices in geometry fingerprints and edit history. Explicit open straight
slanted side edges are extended to a temporary vertical guide, hollowed, then
trimmed with a planar half-space. Trim the cavity too before fitting inserts.
CAD operations use existing cancellation and validation controls.

## Alternatives Considered
Merely overriding the old open flag fails at the zero-height extreme of a
slanted nose. Rotating every profile would change interior loft geometry and
station semantics. Automatically moving a former would change user geometry.

## Consequences
The user controls closure independently of profile location. Outlines remain
closed loops. Straight slanted ends less than 45 degrees from vertical are
supported; pointed/undersized openings still require usable cross-sections.
The original project is not overwritten during diagnostics.

## Validation
Focused GUI/persistence/history tests, explicit end material classification tests,
and a Debug build using a temporary BabyBuzzard36 copy. Results are recorded in
the matching baseline report.
