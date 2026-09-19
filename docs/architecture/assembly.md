# Assembly

Assembly unlocks when Wing assignments, Fuselage outlines/profiles and both
stabilizer outlines/leading-edge choices are complete. Selecting Assembly disables
2D View, activates 3D, hides the secondary toolbar, and shows instructions,
Wing/Horiz Stab/Vert Stab selectors and the bottom Cut Intersections button.
Leaving Assembly restores 2D availability.

An owned preparation worker reuses unchanged component models and generates
missing models from immutable inputs. The fuselage stays at its original origin.
Initial Wing placement centers its X bounds over the fuselage with a gap of
max(5 mm, 3% fuselage length) above it. Tail trailing bounds align with the tail;
Horiz Stab is centered at fuselage mid-height and Vert Stab starts above the body
with the same gap. The camera looks from negative Y with Z up, centered on the
fuselage and scaled to include every component. Re-entering Assembly restores
this side view without resetting positions.

Selecting a part highlights it orange. Left/right translate X toward nose/tail;
up/down translate Z. Steps are 1 mm, Shift 10 mm or Ctrl 0.1 mm, independent of
display units, zoom and camera orbit. Centerline Y remains zero. A stabilizer
and its control surface move together. Selection alone is transient; positioning
affects project dirty state.

Cut Intersections runs in a cancellable background job. It deep-copies shapes
and checks positive-volume intersections (>0.000001 mm3) only between Elevator
and Rudder. A popup reports that pair before any seat is cut. No other pair
blocks cutting. Mere touching is permitted. Deflection envelopes
and structural clearance are not modeled.

Wing and both fixed stabilizers cut the fuselage; the fixed fin also cuts the
fixed horizontal stabilizer. Cuts operate separately on manufacturing solids,
retain resulting valid solids, and reject consuming an entire source part.
Original Wing, stabilizers, control surfaces and component caches are preserved.
Successful cuts disable the three positioning buttons and change the action to
Undo Cuts. Undo restores original shapes at the same positions and enables movement.

`AssemblyState` persists placements and cut intent in format 21. `AssemblyParts`
keeps Fuselage, Wing, fixed horizontal/vertical stabilizers and elevator/rudder
distinct. `StabilizerBuildResult` preserves those roles through user Cut Shapes.
`MainWindow::exportAssemblyParts()` returns the current placed/cut snapshot, or
no snapshot when stale/unavailable; future exporters must use it. File export
actions remain unimplemented. Source-model edits invalidate derived cuts while
retaining physical translations. New/Close resets placements and derived caches. Current format 24 retains input-only saving introduced in version 23. It saves inputs and Assembly placements/cut intent; all geometry
caches are session-only. Former rotation is included in Fuselage snapshots and
cache invalidation. Version-22 embedded models are ignored on Open.

The existing processing lock, Cancel control and project epoch checks cover
preparation and cutting. Cached shapes are never meshed or Boolean-modified by
the cut worker. Assembly workers are cancelled and joined during destruction.
See ADR-0033 and `../formats/foam-project.md`.

The reference image is rendered as textured pages behind the Assembly geometry,
without sketch lines or splines. Page stacking and physical-size/pixel units match
the 2D editor. A shared fuselage side transform maps drawing X to model X and
inverts drawing Y into model Z using the side outline's nose origin and scale.
The image follows model zoom/pan and is excluded from automatic model fitting.
It is presentation only and is not part of collision checks or exported solids.

Opening a project always selects 2D View, regardless of its saved viewport, so
opening alone never regenerates models. A saved Assembly workspace opens in
Fuselage/Outline/Side View instead, preserving Assembly placements and cut intent.
Explicitly selecting 3D View or entering Assembly starts model preparation.

Assembly preparation schedules missing Wing, Fuselage, Horiz Stab and Vert Stab
models concurrently through `runIndexedTasks`, bounded to four workers and the
hardware concurrency. Valid caches are reused. Each task owns its input/result
slot; progress is queued under the background job mutex. Failure cancels sibling
tasks and joins them before reporting; cancellation publishes no partial snapshot.
Wing panel concurrency is limited to one while other missing components are
scheduled, avoiding nested worker pools; a lone missing Wing uses its normal panel
parallelism. The GUI remains locked until the complete Assembly result is ready.

Mouse controls use physical viewport pixels on scaled displays. Wheel-up zooms
in around the cursor, wheel-down zooms out; right-drag pans and left-drag rotates.
