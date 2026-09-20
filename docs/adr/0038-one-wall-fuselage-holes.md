# ADR-0038: One-wall fuselage holes
Status: Accepted
Date: 2026-09-20

## Context
Holes must remove material through one selected fuselage wall while preserving
the opposite wall. A fixed-depth or centre-plane cut is unreliable with varying
wall thickness and asymmetric profiles. Cut and Holes need consistent controls
for multiple paths without changing the existing Cut geometry or saved layers.

## Decision
Store Holes as four ordinary sketch layers: Top, Bottom, Left, Right. Each
connected path is a selectable hole. Top/Bottom map through Top View; Left/Right
through Side View. Left is negative model Y and Right positive Y. Share the
Add, path selector, Line/Spline and Delete controls with the two-view Cut editor.
Require each hole to be a closed loop contained in its projected exterior outline.
Retain incomplete drafts and warn on leaving Holes.

Create a through-prism from each exact Line/Spline wire, then subtract the
original inner cavity. Retain only the resulting cutter solid touching the chosen
exterior starting plane. Reject it if it also reaches the opposite exterior
plane: that footprint cannot be safely cut through one wall. Apply the selected
tool after supports/rails, before Cut and half alignment. Removable inserts stay
unchanged. Cavity clipping limits the hole to the skin rather than clearing
internal supports. Reject empty, invalid and non-removing cuts.

Project version 25 adds fuselageHoles, including editing drafts. Older projects
receive four empty layers; existing fuselageCuts storage remains unchanged.
Hole geometry participates in fuselage/Assembly invalidation and reference remap.

## Alternatives Considered
A fixed depth can miss thick walls or penetrate the opposite wall. A global
centre plane can intersect either wall on asymmetric fuselages. Replacing Cut's
storage with a new collection type would unnecessarily complicate migration.

## Consequences
A hole can fit the exterior outline but still be rejected if its entire footprint
does not reach the cavity, such as near a closed nose or a side edge. Such holes
must be moved or resized. No new dependency or incompatible migration of prior
project inputs is introduced.

## Validation
Focused checks cover each wall and its opposite, multiple holes, splines,
containment, cavity-edge rejection, incomplete paths, cancellation, shared panel
controls, project migration/round-trip and full fuselage generation. Wing
generation remains excluded.
