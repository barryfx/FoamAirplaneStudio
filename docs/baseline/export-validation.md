# Assembly Export validation

Date: 2026-09-19. Windows/MSVC Debug, branch Export.

Built `designrc` and `assembly_tests` successfully with the windows-debug preset.
Before the standalone-insert and default Former STEP changes, the focused CTest run passed all four suites in 14.27 seconds:

| Suite | Seconds |
| --- | ---: |
| assembly_tests | 7.05 |
| export_tests | 4.60 |
| project_persistence_ui_tests | 1.63 |
| file_dialog_tests | 0.92 |

Export tests inject synthetic already-generated component caches, then use the
actual Assembly preparation/cutting workers and Export panel. They verify:

- Disabled Export before Assembly preparation, enabled after generation, and
  invalidation after changed inputs, New and Open.
- Nose-to-tail numbering for out-of-order formers, local planes for rotated
  masks and original caches.
- Independent exclusive format groups, All selection/clearing, individual
  deselection, disabled empty export, hidden secondary toolbar and 3D display.
- Actual directory acceptance, remembered folder, cancellation, one combined
  component STEP and one DXF per former, and former-only partial selection.
- STEP names, valid reimported BREP, matching selected solid count and volume.
- Binary STL structure and triangle volumes for each selected part.
- Closed DXF polylines with millimeter units; inner/outer sections of a rotated
  former with a hole, with physical section areas checked numerically.
- Save from Export and reopen in 2D with no generation or stale Export access.

Inspected `build/debug/export-panel.png`: readable instructions, both radio
groups, All, formers before components, and Export Components at the panel bottom.
The screenshot uses synthetic geometry; no claim is made for a GentleLady export.
The original STEP regression executable contains legacy Wing generation, so it
was not run. Its exporter is exercised by the new focused STEP round-trip check.
Wing/Fuselage generation suites and broad test suites were not run.

The first Export run passed file checks but failed its final synthetic cache
injection because saved cut intent still requested a missing cut snapshot. The
fixture now explicitly injects uncut state and passes; no production change was
needed for that failure. Existing OCCT deprecation and Qt deployment warnings
remain nonfatal. Linux/macOS and native folder-dialog interaction were not run;
tests use Qt dialogs and temporary settings without changing user preferences.

Logs: `build/export-final-build.log`, `build/export-final-tests.log`.
`git diff --check` passed. The rebuilt Debug application was launched after checks.

## Follow-up: standalone inserts and default Former STEP

Formers and the servo tray now bypass Assembly seat cuts and remain independent
from the body. Formers STEP is the default and shares Components.step with any
selected STEP components. Tests were extended for unchanged insert identity,
tray presence, default/exclusive former radios, mixed STEP/STL output, former-only
and component-only selections, and STEP solid count, names and volume.
These new checks were coded but not run, as explicitly requested by the user.
Prior test results and screenshot above do not validate these follow-up changes.

The follow-up Debug application and updated assembly/export test executable compiled
successfully. Build log: build/former-step-build.log. No follow-up tests were run.


## Follow-up validation: 2026-09-20

After rebuilding, all four focused suites passed: assembly_tests (6.17 s),
export_tests (7.03 s), project_persistence_ui_tests (1.28 s), and file_dialog_tests
(0.80 s), 15.40 s total. This validates standalone inserts, default Former STEP,
and mixed/combined export formats. No Wing/Fuselage generation tests were run.
Initial runs reported heap corruption during ExportPanel cleanup; recompiling
the panel and its caller cleared it. Temporary diagnostic instrumentation was
removed before the final successful build and test run. No production logic
change was needed. Logs: build/export-verified-build.log and
build/export-verified-tests.log.
