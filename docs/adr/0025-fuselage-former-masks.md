# ADR-0025: Cavity-fitted formers from Side View masks
Status: Accepted
Date: 2026-09-17

## Context
The user replaces Firewall with multiple full/partial-height formers, sets their
thickness, and requires no overlap with each other, the tray, or fuselage walls.

## Decision
Store positive axis-aligned Side View rectangles in additive project version 15.
Clip full-width slabs against the exact Thicken cavity and remove supported-body
material. Keep formers as separate solids, appended after fuselage cuts. Enforce
positive-area mask collisions in both editors, persistence and generation. The
next-former thickness uses physical mm; rectangle coordinates follow Reference
remapping and use the established Side View generation scale.

## Alternatives Considered
An outer-outline extrusion would penetrate walls. Fusing inserts into the body
would lose part identity. Wall slots/retainers would add unrequested skin changes.
Exact 3D collision during mouse moves would require expensive regeneration;
conservative Side View rectangle tests prevent overlap immediately.

## Consequences
Partial height is controlled by translating the mask vertically or editing its
upper/lower edge. Masks may extend outside the outline and are clipped in 3D.
Touching edges are allowed. A mask missing the cavity or forming disconnected
pieces is rejected during generation. Cut acts on the supported fuselage while
inserts remain separate. Older projects stay readable with no automatic file writes.

The default-thickness revision enables hollowing when inserts are regenerated,
filling missing station walls with the established defaults while preserving edits.
The user need not visit Thicken first.

## Validation
Focused tests exercise full/partial cavity intersection, zero wall/tray overlap,
ledge clearance, tapered cavities, collision rejection, UI editing/deletion,
format migration, visible overlays and independent generation/cache.
See ../baseline/formers-validation.md for results.
