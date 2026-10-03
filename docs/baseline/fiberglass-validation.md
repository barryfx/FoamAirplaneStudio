# Fiberglass validation — 2026-10-03

Windows Debug build of the application and relevant tests succeeded. The scoped
CTest run passed 7/7 in 36.22 seconds:

- `fiberglass_tests`: analytical flat strips, wrapped sidewalls, hollow-shell
  exclusion, circle area, cylinder surface area, open-path clipping/rejection,
  all four component projections, mirrored wings, dihedral, Assembly rotation
  and translation, every fuselage surface, cloth/resin mass and overlapping layers;
  format-32 round trip, version-31 migration and malformed metadata rejection;
  GUI Line/Circle drawing, shape naming, side selection, cloth-unit conversion,
  automatic/manual resin thickness, selection, restoration and deletion.
- `weight_balance_tests`: existing material/part calculations and GUI workflows,
  density-dialog OK/Cancel, fiberglass breakdown and total/CG integration, area
  reuse after cloth edits, unchanged Assembly geometry, undo/redo and save/open.
- `wing_workflow_tests`: Fiberglass availability after outline completion and
  exclusive selection, with the original sequential prerequisites preserved.
- `sketch_editor_tests`, `editor_history_tests`, `view_controls_tests`,
  `startup_gui_tests`: existing editor/navigation/history/startup regressions.

Renderer evidence in the ignored Debug build directory was inspected:
`fiberglass-editor.png`, `fiberglass-wing.png`, `material-densities.png`, and
`weight-balance-fiberglass.png`. Teal patches remain distinguishable from blue
outlines, yellow cuts and violet holes. The material dialog displays all four
densities. Cloth/resin appear separately in the scrollable breakdown and totals.

An early edge-on surface test timed out during development; the final
implementation clips projected intervals directly rather than recursively
subdividing sidewalls. Analytical area tests complete in about one second with
the final build. A mirrored-dihedral projection regression was corrected by
using one affine frame per triangle. GUI regression testing also caught and
removed navigation-time mutation of empty-patch unit preferences.

No Wing or Fuselage generation tests or suites that invoke their builders were
run, as required by AGENTS.md. Tests use analytical OCCT primitives and injected
Assembly fixtures. Linux/macOS and full generated aircraft were not exercised.
Surface integration remains the documented mesh/visibility estimate, not exact
laminate analysis. No generated artifacts are committed.
