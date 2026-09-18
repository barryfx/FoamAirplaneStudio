# Stabilizer hinge lines

Both stabilizer Hinge Line panels explain the operation, then offer mutually
exclusive Tape Hinge Cut (default) and Standard Hinge Cut radio buttons. Draw Hinge
Line starts a connected straight polyline: click its start and successive corners,
then Escape to finish. With drawing off, drag nodes or select a segment and Delete.
Only the active stabilizer accepts input; the associated outline remains visible
and locked. Drawing is disabled in 3D. New/Close clears each independent hinge.
Reference scaling remaps hinge points together with outlines.

Ends must reach or extend beyond the stabilizer boundary, including the root edge.
The path must be one connected open chain and separate exactly two solid bodies.
The longest segment defines the hinge; ties use the first stored segment. Its
trailing-edge side, as established by the selected outline LE endpoint, is the
moving elevator/rudder. A segment parallel to the chord cannot define that side
and is rejected. Invalid or incomplete separation reports an error; it never
publishes an uncut substitute or stale model as a successful cut.

The builder maps the polyline to chord/span model coordinates, splits the single
stabilizer half with finite vertical segment faces, then bevels only the moving
body along the longest segment. Other segments remain square separation cuts.
Tape retains upper-surface contact and cuts a 45-degree gap below. Standard retains
mid-thickness contact and cuts equal 45-degree gaps above/below. The construction
uses the same 33 sampled local heights and relief profiles as wing controls;
curved surface contact is a sampled approximation. It adds no automatic end gap.
Horizontal mirroring follows cutting; each matching fixed/elevator pair is fused
across the centerline (two bodies total); the vertical pair rotates
upright after cutting (two bodies). An empty hinge leaves the original solid(s).

Each existing stabilizer worker captures hinge geometry/style; its fingerprint
includes both. Status reports cutting; Cancel, stale-result rejection and component
camera preservation follow the existing job lifecycle. Version 19 preserves
sketch geometry, draft/tool state and hinge style; older projects have no hinge and
default to Tape. Drawing/navigation selections are excluded from dirty state when
no draft is pending. See formats/foam-project.md and ADR-0030.
