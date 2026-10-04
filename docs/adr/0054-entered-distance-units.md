# ADR-0054: Retain entered units for distance fields
Status: Accepted
Date: 2026-10-04

## Context
Several distance editors reformatted values in project units after every refresh,
losing explicit mm/in choices. Transient per-panel preferences also disappeared
on project restoration and could not represent mixed units on separate items.

## Decision
Keep millimetres as the physical value and store an optional per-field display
unit with the owning model item. Defaults retain existing input conventions;
committing a valid bare number records that convention. Invalid edits retain the
previous value and unit. Metadata is additive in project version 33, includes
undo snapshots, and is excluded from fuselage geometry fingerprints. Existing
reference/spar/lightening text persistence stays in place. Resin thickness and
balance-part dimensions use suffix-aware line edits with physical-range validation.

## Alternatives Considered
Transient widget properties cannot survive Save/Open or reliably track reordered
items. Retaining a duplicate numeric string everywhere would require reconciling
strings with geometry-driven changes; the unit alone is sufficient for display.

## Consequences
Display may canonicalize spelling (inches to in) and numeric precision, but never
converts a committed value to project units merely on refresh or unit changes.
Older files cannot recover previously discarded input units. Older readers can
read the new files but discard optional unit preferences on resave.

## Validation
`length_display_tests` covers mixed units, refresh, item restoration, project
round-trip and invalid metadata. Relevant panel, fiberglass, balance, former,
tray, wall and history tests cover existing workflows.
