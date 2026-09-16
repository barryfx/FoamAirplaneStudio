# ADR-0015: Near-terminal internal stations and full oblique root caps
Status: Accepted
Date: 2026-09-15

## Context
GentleLady has a slightly oblique panel join. Constant-span sampling of the
construction closure collapses its chord and thickness, preventing full-length
spars. The user requests ending geometry at a nearby last station, explicitly
excluding the wing tip, and rejecting conflicting LE/TE definitions immediately.

## Decision
For internal panels only, consider the last station near-terminal when both
endpoint spans are within 1% of the original panel span from its outermost
outline point. End the loft at the lesser endpoint span, perpendicular to the
common span axis. Sample the full outline chord there; omit any oblique exact
station wire that would extend past that cutoff. Keep original scale calibration,
editable outlines, station anchors and file format. Outer panels always retain
the actual tip contour and tip treatment. Farther-inboard stations do not truncate.

At an oblique root, loft from a full nearest-airfoil cap at the open root ends,
then use sections beyond the cap's complete span extent. Use an ordinary
constant-span section for any oblique station touching the root cap, avoiding
overlapping loft wires while retaining its airfoil in interpolation. Spar surface sampling
starts just beyond the complete cap; the cutter extends back through it and the
solid boundary limits the cut. This avoids evaluating skin at a zero-area corner.

Reject newly drawn stations whose LE-to-TE vector has a nonpositive dot product
with any existing station. Display a corrective popup immediately. Validate saved
station directions before generation too, without rewriting user data.

## Alternatives Considered
Truncating all panels would remove intended wing tips. Silently shrinking spars
would hide invalid geometry and change entered lengths. Using a fixed pixel
threshold would depend on reference resolution. Automatically swapping saved
endpoints would assume which station the user intended as LE-first.

## Consequences
Internal ends within the tolerance become equal-span caps. Source outlines remain
available for editing; there is no persistent format migration. Slight root caps
retain their actual obliquity. The groove trajectory over the short oblique root
interval holds its first complete skin sample. Intentional larger station setbacks
and all outermost wing tips preserve previous extent behavior.

## Validation
Station UI tests inspect and dismiss the actual popup and verify no reversed line
is added. Geometry tests cover rejection of saved conflicts, full-length grooves
at a skew internal join, cutoff extent and the outer-tip exception. Existing spar,
wing-solid and project workflow tests remain applicable. The GentleLady copy with
only the reversed station corrected reproduces the real boundary conditions.
Execution results are recorded in docs/baseline/station-boundaries-validation.md.
