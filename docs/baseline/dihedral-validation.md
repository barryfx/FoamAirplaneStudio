# Dihedral and rounded-tip validation

Date: 2026-09-15. Windows Debug, Qt 6.11.1 / OCCT 8.

## Build and tests

Rebuilt Debug with `cmake --build --preset windows-debug --parallel 4`. Checked
for workspace app processes before rebuilding; no app was running during builds.
No DesignRC files were modified. This workspace has no Git repository.

Passing targeted suites:
- wing_solid_tests: 312.87 seconds on the final run, including centerline mating,
  panel-joint sidedness/contact, relative/cumulative angles, mirrored elevation,
  rounded-tip thickness and planform, internal cutoff, profile interpolation,
  invalid geometry and angle rejection.
- project_tests: 81.10 seconds on the final run, including per-panel angle and
  tab preservation, old tip-tool migration with all-zero angles, invalid array
  lengths, all file lifecycle actions, embedded images/drafts and camera state.
- spar_tests: 129.79 seconds; grooves, mid splitting/alignment, per-panel controls
  and mirroring with the new fixed rounded tip.
- station_workflow_tests: 114.67 seconds; Dihedral field availability, angle edits,
  deferred regeneration, retained camera, controls, top/bottom/mid spars and
  view-only navigation not dirtying or regenerating the model.
- spar_panel_tests: 0.36 seconds; also checks the Dihedral widget's independent
  values, zero defaults, tab-only navigation and resize/restore behavior.
- wing_workflow_tests: 0.17 seconds; Dihedral and optional-tool readiness.

The focused assembly test verifies a 3-degree first panel and an additional
5-degree outer panel. For two 250 mm panels, the final axis point is
Y=497.224 mm, Z=47.8773 mm. Points just inside/outside both miter planes and
both mirrored root halves verify mating without gaps or overlapping thickness.

Initial validation found a rounded-TE closure tolerance problem, corrected with
a small geometric closure floor. Later test failures were an obsolete exact
single-panel error-string expectation and an invalid test fixture selecting
Dihedral while retaining active airfoil sketch mode; corrected tests pass.

## Real project and visual smoke

A temporary GentleLady copy uses its previously corrected LE/TE station, original
skewed outline and spar settings, and angles [3,5]. The original project remains
unchanged (SHA256 B393D48F8B7A13FDCCEE41008A6C4FB2A7EBDCB457AC6F11902F0F0E012E2EB6).
The MainWindow smoke path opens and generates the copy, verifies wingModelReady
and unchanged project data, and captures both isometric and front views:

- build/debug/gentle-lady-dihedral.png
- build/debug/gentle-lady-dihedral.png.front.png

Visually inspected both captures: per-panel Root Dihedral controls, raised outer
panels, joined centerline/panel boundaries, rounded tips and existing spar cuts.
The front camera change is only in the test window and is not saved to the source.

The rebuilt Debug app was launched after validation. Existing OCCT deprecation
and Qt deployment VCINSTALLDIR warnings remain. Linux/macOS were not exercised;
no performance improvement is claimed by this change.
