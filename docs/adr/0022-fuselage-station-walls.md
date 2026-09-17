# ADR-0022: Persistent station walls and geometry-driven end closure
Status: Accepted
Date: 2026-09-17

## Context
Thicken must assign decimal wall thicknesses to stations, retain them across
mode changes, and generate a smoothly varying hollow fuselage. User clarification
requires the same rule at both ends: open at an end profile, closed beyond it.

## Decision
Store nullable thickness on each station plus a first-entry activation flag in
project version 12. Prefer a Side View wing-seat edge matching the root chord for
8 mm/5 mm initial defaults, preserving user values thereafter. Generate inward
planar section offsets using OCCT, a monotone smoothstep thickness law, and an
inner loft. Join open rims or add reversed inner cavity shells as the endpoint
geometry requires. Keep all profile sketches visible in 2D.

## Alternatives Considered
Thickness arrays indexed separately from stations risk reassignment on deletion.
A constant whole-body offset cannot implement varying station values. Shrinking
profile bounding boxes does not give a uniform offset around a section. A global
variable surface-normal offset would require a different skin-fitting algorithm;
this implementation defines thickness in the station plane.

## Consequences
Old files remain readable and start with thickening disabled. Existing dimensions
remain attached to stations. The exterior remains unchanged; narrow extended tips
can remain solid. Invalid or pinched cavities report an error instead of silently
reducing the requested thickness. See architecture/fuselage-thickness.md.

## Validation
Focused profile visibility, defaults, decimal editing, persistence/migration,
inward-wall measurement and endpoint classification tests; Debug build and
native visual smoke checks. Results are recorded in the baseline validation note.
