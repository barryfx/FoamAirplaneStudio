# ADR-0029: Former retainers and main fuselage halves
Status: Accepted
Date: 2026-09-18

## Context
Formers need locating shoulders when the fuselage side halves are joined.
Hatches and other cut-out parts must remain whole. Saved cut paths do not carry
part roles or a user-selected main-body identity.

## Decision
Generate 4 mm fore/aft by 3 mm inward retaining rails immediately ahead of and
behind every former, on both cavity sides. Use cavity intersections and lateral
translations to follow full available side height. Subtract removable inserts
from rails, then fuse rails to the shell before user cuts. Require a connected,
valid body. Keep partial-height formers and tray solids separate.
After user cuts, identify the largest-volume solid as the main body and split it
on the registered Y=0 longitudinal plane through the aligned noses. Preserve
other cut solids unchanged. Reject equal-largest ambiguity and splits that do
not produce exactly two connected valid halves. Check material conservation.
These are derived features; no project fields or format migration are required.

## Alternatives Considered
Splitting before cuts would also split detached hatches. Bounding-box rails
would not follow curved inner walls. Explicit body-role selection would handle
unusual cuts better, but requires a new selection workflow and persistent data.

## Consequences
Ordinary smaller hatches remain whole. The largest-solid rule assumes the main
body is larger than each detached piece. Highly asymmetric or multiply connected
centre intersections can fail with an explanation. Rails are clipped around
inserts and unavailable cavity regions, so full-height contact is not guaranteed.
Fixed dimensions are physical millimetres, independent of Reference display units.

## Validation
Debug build, editor checks, and subsequently user-authorized Fuselage regeneration
checks are recorded in ../baseline/former-retainers-validation.md. Focused tests
cover rail dimensions, material conservation, whole cut-out identity, formers,
tray, optional defaults and cache reuse. Wing generation tests remain excluded.
