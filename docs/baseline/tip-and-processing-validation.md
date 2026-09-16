# Wing tip, project reset and processing validation

Windows Debug, 2026-09-13.

- The seven selected tests passed (146.35 seconds): project_tests,
  wing_solid_tests, airfoil_panel_tests, station_workflow_tests,
  wing_workflow_tests, reference_tests and designrc_gui_tests.
- New from a populated, saved Wing project disables every main workspace action
  except Reference and clears the previous model.
- Vertical rays just inside rectangular and rounded tip contours hit the solid
  after beveling; rounded contours are checked for all three tip choices.
  Solid validity, mirroring, volume, scale, panels and oblique roots also pass.
- UI tests observe the wait cursor during processing stage messages and confirm
  its restoration afterward. Failed saves preserve data and restore the cursor.
- The mirrored flat-bottom wing rendered in the OCCT viewport; the smoke capture
  is build/debug/tip-fix-smoke.png (generated output, not a source asset).
- Debug compilation succeeded. Existing OCCT deprecation and Qt deployment
  environment warnings remain; Linux and macOS were not tested.

No changes were made in DesignRC. ADR-0007 documents the contour-following bevel
and its approximation limits; the persistent project format is unchanged.

After the final status-message adjustments, project_tests, station_workflow_tests
and reference_tests passed again (3/3, 15.12 seconds).
