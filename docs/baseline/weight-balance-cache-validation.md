# Incremental Weight and Balance validation — 2026-10-03

Windows Debug targeted tests passed: `fiberglass_tests`, `weight_balance_tests`
and `airplane_statistics_tests` (3/3, 20.10 seconds). The expanded GUI cache
regression then passed in `weight_balance_tests` (9.93 seconds).

Coverage includes:

- Worker-budget rounding for unknown, 1, 2, 4, 8, 16, 20 and 64 logical threads.
- Serial/parallel/cached parity for material volume, area, mass, centroid and
  deterministic row order using analytical OCCT primitives.
- Ten fiberglass patches, including eight on the same fuselage fixture; all
  independent misses can be scheduled concurrently rather than by component.
- One boundary edit performs one additional patch measurement. Adding a patch
  measures only that patch; deleting/reordering patches reuses surviving results.
- Name, cloth weight and resin thickness edits reweight existing measurements.
  A Top View edit does not invalidate Side View patches.
- Wrap/surface changes invalidate the changed patch; horizontal placement changes
  invalidate only horizontal covering. Repeated numerically equal placement
  transforms reuse results despite new TopLoc datum identities.
- New fuselage topology with identical bounds invalidates its eight patches,
  while stabilizer results are reused. Scale changes invalidate all dependent
  projections. Invalid open paths leave the previous valid cache intact.
- Individual volume reuse after component edits, insert renaming/reordering,
  and placement changes, with parity against an uncached measurement.
- MainWindow integration verifies that adding/editing a fiberglass patch does
  not reintegrate material volumes or other patches, density changes reuse both
  caches, deletion removes obsolete entries, and reopening clears transient data.
- Existing GUI, persistence and deferred-statistics checks remain passing.

The machine reports 22 logical hardware threads; the production budget is 19
(`floor(22 * 0.9)`). The ten-patch fixture supplies ten independent tasks.
One timed call per case, Windows Debug:

| Calculation | Time |
| --- | ---: |
| Cold, serial | 210.138 ms |
| Cold, normal worker budget | 204.525 ms |
| All measurements cached | 0.8614 ms |
| One boundary edited | 23.0813 ms |

These measurements record small-fixture behavior, not a production-aircraft
speedup guarantee. The geometry/visibility approximation is unchanged. Parallel
tasks own private meshes and classifier state and reduce in deterministic order.

An incremental test link reported LNK1236 for the generated Fiberglass object.
Removing only the affected object/library and test incremental-link artifact,
then rebuilding the test target, resolved the error; the expanded test passed.

After the user's subsequent build/run request, the Debug `designrc` target built
successfully and the rebuilt `foamairplanestudio.exe` was launched and confirmed
responsive. Qt deployment emitted its existing `VCINSTALLDIR` warning.

No Wing or Fuselage generation tests were run, per AGENTS.md. Linux/macOS and
large-aircraft memory/performance remain untested. No generated artifacts are
committed, and no project-file format change was made.
