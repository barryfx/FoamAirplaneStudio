# ADR-0020: Single-click vertical fuselage stations
Status: Accepted
Date: 2026-09-16

## Context
Fuselage Profile Stations needs floating boundary points and one-click vertical
section placement, while retaining the established Wing station editing behavior.

## Decision
Add an opt-in vertical placement mode to ConstrainedLineEditor, enabled only for
the independent fuselage sketch's station collection. Use the Side View layer.
Intersect all displayed curves at the hovered X, deduplicate vertex hits and accept
exactly two distinct boundary points. Store upper then lower anchors; enforce
verticality during placement, endpoint movement, outline edits and decoding.
Wing's default two-click station mode is unchanged.

Use version 10 to store independent fuselageStations. Read versions 1-9 with an
empty collection. Keep hover/preview transient and selection outside dirty checking.
Reuse the existing lifecycle, reference remapping and station rendering.

## Alternatives Considered
Two clicks would contradict one-click placement. Searching only another curve
fails when a closed spline defines both edges. Choosing outermost intersections
through a folded outline could bridge exterior space. A separate duplicated
controller would diverge from existing selection, deletion and attachment rules.

## Consequences
Only unambiguous nonzero-height sections can be placed. Near pointed ends the user
must choose a location with finite height. Geometry uses existing display sampling;
there is no fuselage solid or profile-shape editor yet. Side View placement does not
infer registration between separately drawn Top and Side outlines. Older files are
preserved until Save, and older applications reject version 10.

## Validation
The focused Fuselage suite covers upper/lower one-click placement, closed splines,
vertical moves, duplicate prevention, deletion, locking, persistence, migration and
ambiguous-section rejection. Results are in baseline/fuselage-station-validation.md.
