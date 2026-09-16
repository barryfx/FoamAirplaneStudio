# ADR-0005: Wing solids from planform sections and assigned airfoils
Status: Accepted
Date: 2026-09-13

## Context
The drawing workflow now needs a mirrored OCCT wing body, selectable tip shapes,
and regeneration after edits without reviving the removed rib-generation pipeline.

## Decision
WingSolidBuilder consumes a value snapshot of panel outlines, curve-attached
stations and their library assignments. OCCT constructs closed section wires and
a solid loft. The result contains two mirrored half-wing solids in a compound;
the root interface remains available for later manufacturing operations.

Model coordinates are millimetres: X chordwise from LE toward TE, Y spanwise,
Z up. The first panel's two open root endpoints establish the chord direction,
oriented LE-to-TE using the innermost station. This is independent of station
creation order and does not require a station exactly at the root. The right half
extends toward positive image X (positive image Y for a vertical drawing).
The line joining those root endpoints defines the root mirror plane Y=0. Manual
wingspan scales the complete mirrored span; actual-scale scenes are already mm.
This does not equate image width with aircraft span.

Sections include every station and sampled outline vertex plus 32 uniform span intervals
(updated by ADR-0007).
Intermediate LE/TE positions intersect the displayed planform curves. At assigned
stations the exact LE-to-TE chord is retained, including oblique stations. Airfoil
ordinates interpolate linearly between assigned profiles and extend unchanged
beyond the first/last assignment. OCCT interpolates upper/lower airfoil curves
through 25 cosine-spaced samples per surface; ruled spans connect sections.
Sharp trailing edges use a two-millionths-of-chord closure for consistent topology.

The model closes open panel roots and inner-panel ends with temporary straight
edges for section intersection. These edges are never added to the sketch.
Sections are restricted to the outboard side of the root plane. This avoids
mistaking an offset root endpoint or an angled first station for an interior
zero-chord closure. Genuine interior pinches remain invalid.

Traced library boundaries use LE left, TE right and screen Y down. Rotate their
closed traversal to the TE, subtract the LE-to-TE baseline and normalize by chord.
Imported profiles retain their existing normalized coordinates.

The default tip is blunt. ADR-0007 supersedes the original global 45-degree
clipping plane with a contour-following bevel that retains plan coordinates.
The default counts as a defined tip without requiring a click.

MainWindow marks the model dirty after outline, station, assignment, library,
reference-scale or tip edits. It coalesces signals and rebuilds only when Wing
and the 3D tab are active. Invalid/incomplete inputs clear stale geometry and
report the reason in the status bar. New resets the default tip and model.

## Alternatives Considered
The inherited rib/wood structure pipeline does not implement solid foam wings.
Lofting only assigned stations would ignore intervening traced planform changes.
A fused full-wing solid would erase the useful root interface. A persistent
model format and background job system are deferred; no new dependencies are added.

## Consequences
The model is a B-rep, with display triangulation generated before publication.
Curved planforms use the sketch display sampling and finite span sections; this
is an approximation, not an exact analytic sweep of every outline spline.
Unusual folded/overlapping planforms or crossing stations can fail validation.
Traced airfoils must be drawn horizontally. CAD work currently runs synchronously
on the UI thread, so complex inputs can pause interaction during regeneration.
Project persistence was subsequently added in ADR-0006; downstream manufacturing
splits remain future work.

## Validation
WingSolidTests checks solid validity, mirrored bounds/volume, physical scaling,
all three tips, multiple panels, profile interpolation, traced profiles and
missing assignments. StationWorkflowTests covers automatic selection, Escape,
tip exclusivity/default assets and deferred/live 3D regeneration. A captured
Windows OCCT viewport provides visual smoke validation.

ADR-0013 supersedes the whole-wing loft and cross-panel control ownership portions: panels now generate from their own stations and control settings. Other geometry rules remain in effect.
