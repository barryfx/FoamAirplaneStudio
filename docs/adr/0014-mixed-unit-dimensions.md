# ADR-0014: Preserve entered dimension units
Status: Accepted
Date: 2026-09-15

## Context
Users need to mix inch and millimetre dimensions without the data panel converting
their entries to the project default. Saved projects must retain this presentation.

## Decision
Keep geometry dimensions in millimetres. Share the decimal/suffix parser between
reference and spar fields. Accept mm/in, spelled aliases and double quote for
inches. Bare numbers use project units. Before a project-unit change, make valid
bare reference values explicit; committed spar values always carry a suffix.
Keep reference raw text in existing fields and add spar sizeText/heightText in
format version 6, with backwards readers for versions 1-5. Verify stored spar text
matches its canonical length. Use text fields for spar physical dimensions;
percentage fields remain spin boxes. Commit spar lengths on Enter/focus loss,
restoring the previous value for invalid entries, as with the prior spin boxes.
Display changes dirty saved project state but are excluded from wing geometry
fingerprints. Untouched spar defaults continue to follow project display units.

## Alternatives Considered
Converting all fields to project units conflicts with the requested behavior.
Keeping only raw strings would spread unit parsing into geometry and complicate
migration. Separate per-field unit selectors would require more panel controls.

## Consequences
A project can display several unit systems concurrently. Older apps cannot read
version 6, while this app retains migration support for their files. No geometry
coordinate system or dependency changes are required.

## Validation
Reference, spar panel, viewport and project lifecycle regression tests cover
mixed-unit entry, preservation, migration and canonical dimensions. Execution
results are recorded in docs/baseline/mixed-units-zoom-validation.md.
