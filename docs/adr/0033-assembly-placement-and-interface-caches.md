# ADR-0033: Assembly placement and reversible interface cuts
Status: Accepted
Date: 2026-09-19

## Context
The Assembly workspace must position existing components, cut mating seats,
reject rudder/elevator collisions, preserve original parts and supply cut geometry
to future export. Components previously exposed only whole display shapes;
Cut Shapes can split a stabilizer into multiple solids, making solid indices
insufficient to identify the elevator or rudder after generation.

## Decision
Use fuselage coordinates (X nose to tail, Y transverse, Z up) with three physical
X/Z translations and a fixed fuselage. No rotation or lateral positioning is
introduced. Preserve fixed and moving stabilizer roles through mirroring,
Cut Shapes and orientation in a structured build result. Retain the existing
shape-returning builder as a compatibility wrapper.

Assembly preparation reuses current component caches and generates only missing
or changed components on an owned background worker. Cut processing deep-copies
the placed shapes, checks only Rudder against Elevator before any subtraction,
then cuts Wing/Horiz Stab/Vert Stab from each fuselage solid and Vert Stab from
each fixed horizontal stabilizer solid. Moving controls remain unchanged.
Retain all positive-volume valid remaining solids; consuming a whole source
manufacturing part is an error. Face-only contact is permitted.

Keep component caches untouched. A separate cut snapshot supplies the assembled
display and `MainWindow::exportAssemblyParts()`. Undo drops this snapshot and
unlocks positioning at the same translations. Source edits invalidate the
Assembly snapshot. Cancellation, errors and stale jobs never publish partial cuts.

Version 21 stores physical translations, initialization state and the requested
cut/uncut state. Versions 1-20 load unpositioned and uncut. Geometry remains transient. ADR-0035 retires version-22 persistent caches:
version 23 saves inputs only and ignores old embedded geometry. No new
dependency or incompatible migration of existing input files is introduced.

## Alternatives Considered
Inferring control identity from output order after arbitrary cuts is unreliable.
Mutating component caches would make Undo and component editing destructive.
Saving BREP copies duplicates derived data and makes files much larger.
Screen-dependent movement after camera orbit makes saved physical positioning
ambiguous; keys instead retain the initial side-view X/Z axes.

The Assembly reference image uses textured pages in an underlay, sharing the
fuselage drawing-to-model alignment. Sketch geometry remains hidden.

## Consequences
Assembly is 3D-only. Arrow steps are 1 mm, Shift 10 mm, Ctrl 0.1 mm. Placement
begins above/near the fuselage as requested and remains user-controlled.
Booleans create exact zero-clearance seats at the chosen positions; no hardware,
adhesive allowance or automatic fin trimming is inferred. Collision detection
checks resting solid overlap, not the sweep of a deflected control surface.
Export UI remains a separate workflow; its geometry source is the Assembly accessor.

## Validation
See `../baseline/assembly-validation.md` for executed checks and limitations.

Assembly preparation schedules missing Wing, Fuselage, Horiz Stab and Vert Stab
models concurrently through `runIndexedTasks`, bounded to four workers and the
hardware concurrency. Valid caches are reused. Each task owns its input/result
slot; progress is queued under the background job mutex. Failure cancels sibling
tasks and joins them before reporting; cancellation publishes no partial snapshot.
Wing panel concurrency is limited to one while other missing components are
scheduled, avoiding nested worker pools; a lone missing Wing uses its normal panel
parallelism. The GUI remains locked until the complete Assembly result is ready.

## Root rotation extension (2026-09-23)
Assembly now adds a pitch angle per movable component about its source root chord
midpoint. The fixed-Y side-view convention matches translation controls; positive
rotation is clockwise from negative Y. Controls share the fixed stabilizer pivot.
Store optional rotationDegrees alongside offsets, defaulting missing angles to
zero. Geometry caches stay immutable; exports and balance use the same transform.
The UI steps by 0.5 degrees and shows the selected angle below its rotation buttons.

## Former wing seats extension (2026-09-23)
Cut Intersections also subtracts the placed wing from private copies of formers,
identified by their existing manufacturing-plane metadata. Retain names, IDs and
planes in the derived snapshot, including multiple valid remaining solids. The
servo tray and source caches remain unchanged. The existing cancellation and
whole-part-consumption checks apply. Undo discards the cut former geometry with
the rest of the cut snapshot. Regression coverage was initially left unbuilt and
unrun at the user's request; the subsequent DAT export task authorized validation.
Debug Assembly and export suites now pass; see the Assembly validation baseline.
