# Fuselage Holes validation
Date: 2026-09-20. Windows/MSVC Debug, branch Export.

Implemented four-wall Holes and shared Add/selector/Line/Spline/Delete controls
for Holes and Cut. Project format 25 round-trips all four wall layers and loads
older projects with empty holes. See ADR-0038 and architecture/fuselage-holes.md.

Successful focused checks:

| Suite | Seconds |
| --- | ---: |
| fuselage_holes_tests | 8.16 |
| former_geometry_tests | 4.95 |
| fuselage_cut_tests | 7.93 |
| assembly_tests | 6.90 |
| export_tests | 6.75 |
| project_persistence_ui_tests | 1.48 |

Holes checks cover off-centre positioning on each wall, exact removed volume,
intact opposite/mirrored positions, multiple holes, a periodic spline, incomplete
loops, outline and cavity-edge rejection, cancellation, whole-path deletion,
warning dialogs, four wall controls, version migration, save/reopen, and full
fuselage generation. The GUI generation fixture reports zero Wing generation.
The Cut suite also performs fuselage generation; no Wing generation suite ran.
The former geometry checks additionally validate the preceding rail-clearance fix.

The initial Holes UI fixture omitted prerequisite Wing definitions, so the
Fuselage workspace was unavailable and its empty toolbar triggered an assertion.
The fixture now provides definitions without generating Wing geometry. During
review an off-centre Top View sign mismatch was fixed and protected by explicit
opposite/mirrored-location assertions. Final checks above passed.

Inspected build/debug/holes-panel.png: readable instructions, Holes secondary tab,
four wall selectors and shared path controls. The screenshot uses the synthetic
fixture; GentleLady holes were not regenerated during this task.

Debug application rebuilt successfully. Logs: build/holes-final-build.log,
build/holes-final-tests.log and build/holes-regressions.log. Tests and screenshots
use isolated settings; the user's project and STEP export were not overwritten.
