# Station direction and near-terminal panel validation

Date: 2026-09-15. Windows Debug, Qt 6.11.1 / OCCT 8.

Rebuilt with `cmake --build --preset windows-debug --parallel 4`. Checked for
workspace app processes before building; none were running. No DesignRC changes.

## Regression evidence
- station_sketch_tests: real popup appears on the second click of a reversed
  station; the station is rejected, no pending point remains, and drawing resumes.
- spar_tests passed (85.32 seconds): surface grooves, mid split/alignment,
  panel ownership, control isolation, dimensions and mirroring.
- station_workflow_tests passed (81.30 seconds).
- project_tests passed (88.18 seconds).
- Initial new wing boundary test exposed an overlapping near-root station wire;
  the implementation now samples an ordinary section there. The focused test then
  passed, using OCCT's tight shape bounds rather than its conservative bounding
  box to verify the exact cutoff: measured/expected 249.59 mm.
- Final wing_solid_tests passed (137.64 seconds), and station_sketch_tests passed
  again (0.30 seconds). The wing suite includes all
  three tip shapes, curved tip contours, scaling, interpolation, panels, invalid
  geometry, LE/TE rejection, full-length spars on skew panel ends, a retained
  outer tip despite a near-end station, and preserved intentional internal setback.

## GentleLady reproduction and visual smoke
Original SHA256 remains
B393D48F8B7A13FDCCEE41008A6C4FB2A7EBDCB457AC6F11902F0F0E012E2EB6.

The original file now fails early with an explicit station 2 / station 1 LE/TE
conflict diagnostic on panel 1. A temporary copy swaps only station one's anchors;
it does not align the outline or change spar sizes/lengths. The Debug spar runner
builds four valid solids through all grooves, mirroring and meshing.

The real MainWindow project smoke path (`FOAM_PROJECT_SMOKE_FILE` in project_tests)
opens that copy, generates and displays 3D, verifies wingModelReady and clean project
state, and captures build/debug/gentle-lady-station-boundaries.png. Visually inspected
that capture: the generated wing and spars display, including its retained tip.
The saved camera is retained, so the capture shows the user's zoomed view.

Existing OCCT deprecation and VCINSTALLDIR deployment warnings remain. Linux and
macOS were not run. The original project file has not been rewritten.

Rebuilt Debug app launched successfully after validation.
