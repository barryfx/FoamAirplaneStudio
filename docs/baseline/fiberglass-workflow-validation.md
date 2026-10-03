# Fiberglass workflow follow-up — 2026-10-03

Windows Debug targeted CTest run: 6/6 passed in 31.80 seconds.

- `fiberglass_tests`: all four initial tabs follow Reference cloth units without
  changing physical cloth weight; explicit overrides persist; additive format-32
  migration and older resin defaults work. Real Line drawing opens the expected
  wrong-view dialog, repeated edits suppress duplicate warnings, and correcting
  the drawing surface clears the warning. Both mismatch directions are checked.
- The open fuselage Side View fixture is a 200 × 80 × 20 mm box. An open U
  closing below the outline covers 20 × 5 mm on each side and 20 × 80 mm on the
  bottom: 1800 mm² total, centroid X=60 mm and Z=-9.722222 mm. The top contributes
  nothing. One Sided covers 100 mm² at Z=-7.5 mm. A mixed spline/line boundary
  produces the same result. No Wing or Fuselage builder is invoked.
- `weight_balance_tests`: analytical material volume/centroid parity, density
  dialog, identical cloth/resin area labels, material-only cache reuse, and no
  integration after Fiberglass edits. Entering Weight and Balance updates mass.
- `airplane_statistics_tests`: every other workspace displays three dashes and
  leaves the measurement cache untouched. Outline statistics still update;
  Weight and Balance recomputes weighted statistics from valid inputs.
- `editor_history_tests`, `view_controls_tests`, `startup_gui_tests`: project
  history, navigation and GUI startup regressions pass.

Serial/four-worker covering comparison on three analytical components:
73.5758 ms / 70.7788 ms (one timed call each). Areas, cloth masses, resin volumes,
centroids and row order match; foam/plywood integration also has serial/parallel
parity assertions. These small fixtures establish correctness and record timing,
not a production-aircraft speedup claim. Geometry and projection state remain
local to each worker; reductions follow input order.

Inspected GUI captures in the ignored Debug build directory confirm project
cloth units and three dashes in Fiberglass, plus the updated material breakdown.
Source documentation and bundled help describe the 1500 kg/m³ resin default,
equal covering areas, unit-following preference, view warnings and deferred mass
calculations. Saved user density overrides are preserved.

The Debug `designrc` target rebuilt successfully and the rebuilt
`build/debug/Debug/foamairplanestudio.exe` was launched. Qt deployment reported
its existing `VCINSTALLDIR` warning; the executable started and responded.

Wing/Fuselage generation suites remain excluded under AGENTS.md. Linux/macOS
and full generated-aircraft performance were not exercised. Surface areas remain
the documented mesh/visibility estimate. No generated artifacts are committed.
