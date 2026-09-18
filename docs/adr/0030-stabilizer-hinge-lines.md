# ADR-0030: Stabilizer polyline hinge separation
Status: Accepted
Date: 2026-09-18

## Context
Both stabilizers need connected line-segment cuts and wing-style hinge relief on
the longest segment, retaining separate elevator/rudder bodies.

## Decision
Reuse the SketchEditor with opt-in continuous Line drawing. Store independent
hinge sketches and Tape/Standard choices in project version 19. Split the existing
stabilizer solid with finite vertical faces and identify the trailing moving body
by solid classification beside the longest segment. Apply the existing sampled
45-degree hinge profile convention locally, before component mirroring/orientation. Fuse corresponding horizontal halves separately across the centerline to retain one fixed body and one elevator body.

## Alternatives Considered
Wing control rectangles cannot represent connected return cuts. Extending every
segment as an infinite cutting plane would split unrelated portions of the body.
Using centroids to classify bodies is unreliable for bent polylines.

## Consequences
No new dependency, Wing-generation path, or assembly architecture is needed.
Open ends must reach the outline, and cuts must yield exactly two connected bodies.
Relief follows sampled upper/middle thickness; other segments remain square.
Existing projects remain readable and initialize empty hinge sketches.

## Validation
Stabilizer-only tests cover Tape/Standard relief on a constant-thickness solid,
longest-segment behavior, reversed traversal, invalid cuts, UI exclusivity,
continuous drawing, draft persistence and horizontal/vertical body counts.
