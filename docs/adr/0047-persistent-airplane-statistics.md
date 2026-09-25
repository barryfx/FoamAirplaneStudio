# ADR-0047: Persistent airplane statistics
Status: Accepted
Date: 2026-09-25

## Context
Airplane statistics must be available throughout editing, survive Save/Open,
and update with data changes without triggering model generation.

## Decision
Compute nominal planform dimensions from scaled sketches. Place shared statistics
footers inside each data panel above bottom actions, excluding Weight and Balance.
Use aggregate Assembly material volumes/centroids and the placed wing datum for
weight, CG and wing loading. Persist these density-independent measurements with
a source key, permitting density/added-part changes after reopening. Validate
the key against existing generation inputs and Assembly offsets, rotation and
cut state; invalidate measurements when those inputs change. Derived values do
not create undo steps or independently dirty a project.

Add an optional `airplaneStatistics` object to format 29. This is a discardable
cache, so earlier readers can safely ignore it. No geometry is serialized.
Weight and Balance retains its existing requirement for current Assembly data
and gains a Wing Loading line; the saved summary does not fabricate its per-part
table when solids are absent.

## Alternatives Considered
Saving only formatted text loses unit conversion and cannot update changed
mass inputs. Regenerating CAD to show a summary would delay editing and opening.
Persisting all solids would reintroduce the intentionally removed model cache.

## Consequences
Opening a saved model can display matching weight/CG/loading without a rebuild.
Measurements omit fabrication allowances and describe nominal planform area.
Stale mass values disappear rather than appearing as current results.

## Validation
A dedicated non-generation suite checks outline dimensions, scale/area factors,
units, wing loading, persistence, invalidation, and GUI footer positioning.
