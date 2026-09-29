# ADR-0049: Four-surface fuselage cuts with retained hatches
Status: Accepted
Date: 2026-09-26

## Context
The user requested selecting all four fuselage surfaces, through-cuts for paths
crossing the outline, and single-wall cuts for interior paths. Both kinds must
retain the interior cut-out as a new component.

## Decision
Use four cut layers matching Holes: Top, Bottom, Left, Right. Classify each
connected path independently against its corresponding 2D outline. For an
interior path, subtract the actual inner cavity from the cutting sheet and keep
only faces connected to the selected exterior plane. Split the fuselage with
that sheet and retain all solids. Reject paths that cannot isolate one wall.
Migrate legacy Side sketches and active drafts to Left in format 31. Legacy
interior paths adopt the explicitly requested new single-wall behavior.

## Alternatives Considered
Stopping at a fixed mid-plane can cut the opposite skin in asymmetric sections.
Using the hole tool would remove the hatch instead of retaining a component.

## Consequences
Interior hatch cuts require hollow geometry and a closed path. A separate
cut-out remains available for inspection/export; existing main-half grouping
and alignment operations still apply. New project end defaults are Nose Open
and Tail Closed; saved choices are retained. No worker ownership model changes.

## Validation
Four-surface tests check hatch position, independent component count, validity,
and conserved volume. Through-cut cases cover all four surfaces. GUI tests
cover view selection, defaults and migration. The full Debug regression suite
is run for this change.
