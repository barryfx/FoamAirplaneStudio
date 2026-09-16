# ADR-0004: Airfoil library and station assignments
Status: Accepted
Date: 2026-09-13

## Context
Users need named imported or traced airfoils and independent assignments to
station lines. Profile tracing must not modify the wing's outline or stations.

## Decision
AirfoilPanel owns an append-only, per-project-window AirfoilLibrary. Entries
contain a display name, imported AirfoilProfile or a sketch snapshot, and ordered
boundary coordinates. Each station retains an optional library index; assignments
therefore move with the station and survive deletion/reindexing of other stations.
No station-index-to-profile map is maintained separately.

PlanViewport owns a second instance of the reusable SketchEditor for airfoil
tracing. Each valid trace occupies its own layer. A temporary layer is validated
as exactly one closed, nondegenerate loop before entering the library; invalid
drafts are discarded after a warning that does not prevent exit. Periodic splines
allow an airfoil to be represented by one closed spline as well as joined curves.

ConstrainedLineEditor has a selection-only mode for airfoil assignment. It
highlights one station and reports selection without moving or deleting geometry.
The DAT loader reuses the domain importer; the GUI loader supplies unnamed-file
fallbacks, recognizes Selig and Lednicer layouts and validates resampling.

## Alternatives Considered
Adding airfoil traces to wing layers would alter outline completion and station
constraints. Duplicating a sketch implementation would diverge in behavior.
Copying airfoil profiles into every station would prevent consistent reuse.

## Consequences
This decision initially kept state in memory. ADR-0006 subsequently adds `.foam`
project persistence for the library, assignments and drafts.
Imported coordinates are normalized. Traced coordinates retain their scene-space
orientation and scale; ADR-0005 subsequently defines horizontal chord normalization
for solid generation. Stored traced boundaries have the display tessellation's
accuracy. Existing completed traces are immutable library snapshots.

## Validation
AirfoilPanelTests exercises the shared file dialog, imported names and fallback,
Lednicer input, shifted-coordinate normalization, independent station assignments,
exclusive selection, locked station geometry, named line and periodic-spline
traces, invalid-loop warnings and cancellation. MainWindow integration verifies
Airfoils controls and Wing Tip availability only after every station is assigned.
