# ADR-0003: Curve-attached airfoil station lines
Status: Accepted
Date: 2026-09-13

## Context
Stations locate interpolation profiles by joining user-identified LE and TE
positions. Their endpoints must slide along existing outlines without editing
those outlines. The same constraint interaction can serve future sketch tools.

## Decision
SketchEditor owns a reusable ConstrainedLineEditor with a separate collection of
two-point lines. Each endpoint retains a source panel/curve index, a normalized
arc-length parameter on its displayed fitted path, and its scene position.
Each line stores Free, Horizontal or Vertical alignment. LE is the first anchor,
TE the second; the application does not infer aerodynamic edge labels.
Hit testing and axis intersections use the same OCCT-fitted path tessellation as
outline rendering. There is no new dependency or project file format.

## Alternatives Considered
Adding station edges to outline layers would change outline connectivity and
allow accidental outline editing. Storing detached coordinates would lose the
constraint needed to move endpoints along their original source curves.

## Consequences
Stations remain in project-window memory for later interpolation and survive
mode changes. At introduction, Project Open/Save was unimplemented; closing the app lost
this project state. ADR-0006 now preserves stations in `.foam` files.
Endpoint evaluation currently has display tessellation
accuracy; manufacturing-grade curve evaluation belongs to future geometry work.
Removing an attached source curve/panel removes dependent stations. Moving a
source curve updates its attachments; a station whose axis constraint can no
longer intersect the paired curve is invalidated. New clears all stations.

## Validation
Station sketch tests cover projection, axis snapping, movement, cancellation,
selection/deletion, spline attachments, mode locking and source deletion.
An integration test drives Reference prerequisites, Wing Outline and Airfoil
Stations through MainWindow and checks that two stations unlock Airfoils.
