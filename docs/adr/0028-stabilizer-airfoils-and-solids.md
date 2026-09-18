# ADR-0028: Single-airfoil stabilizer solids with independent background jobs
Status: Accepted
Date: 2026-09-17

## Context
Both stabilizers now need a DAT selector, the supplied NACA009 default and automatic
3D generation without station assignments. They must share Cancel behavior while
preserving independent component data and caches.

## Decision
Bundle the supplied DAT as a Qt resource. Use the existing importer and file dialog
for one independently selected airfoil per component. Persist custom normalized
coordinates and names in version 17; null or older files select the bundled default.

Use a dedicated OCCT section-loft builder under the existing geometry architecture.
The open endpoints determine the root chord, and the traced outline determines local
chords. Apply the same airfoil throughout. Mirror only the horizontal stabilizer;
rotate the vertical fin upright. Manual reference scaling uses Fuselage Length and
the traced Side View extent. Record coordinate, sampling and tip conventions in
architecture/stabilizer-solids.md.

Own a BackgroundJob and shape/fingerprint cache for each stabilizer, and extend the
existing one-job editing lock, status queue, Cancel, epoch checks and close behavior.
Gate all non-Outline actions until the outline passes drawing readiness.

## Alternatives Considered
Hard-coding a local path would break deployment. A synthetic NACA generator would
not reproduce the user's smoothed DAT. Reusing Wing generation through fabricated
stations would couple component behavior to unrelated panel and wing settings.
Synchronous generation would block Cancel and UI progress.

## Consequences
No new dependency or replacement geometry architecture is introduced. Version 17
is required for custom stabilizer selections; versions 1–16 remain readable.
Sampled lofts have the documented limits. Manual drawings must share Side View scale.
Hinge Line, Cut and assembly positioning remain future work.

## Validation
Focused stabilizer tests cover solid validity, mirroring, physical scale, rotated
outlines, flat tips, defaults, airfoil replacement/persistence, gating, cancellation,
cache reuse and component isolation. See baseline/stabilizer-model-validation.md.
Wing and Fuselage generation tests are excluded per the user's instruction.
