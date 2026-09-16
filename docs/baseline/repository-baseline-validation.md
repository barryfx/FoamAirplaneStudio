# Repository baseline validation

Date: 2026-09-16. Windows Debug, MSVC, Qt 6.11.1 / OCCT 8.

Updated README, installed Help, current architecture/format/requirements wording,
and the documentation guide to reflect dihedral, version 8, lightening and
cancellable background regeneration. Historical validation and DesignRC provenance
remain unchanged. No application algorithms or persistent format changed.

Validation for this documentation/baseline snapshot:

- Full Debug build passed using `cmake --build --preset windows-debug --parallel 4`.
- Ten focused CTest suites passed in 5.03 s: reference_tests, wing_workflow_tests,
  designrc_gui_tests, sketch_editor_tests, station_sketch_tests, airfoil_panel_tests,
  spar_panel_tests, control_surface_editor_tests, processing_tests, file_dialog_tests.
- Rebuilt Debug application launched. Workspace executable processes were checked
  and stopped before building; unrelated processes were preserved.
- Existing VCINSTALLDIR deployment warning remains nonfatal.
- Geometry suites were not repeated for documentation-only changes. Their latest
  eight-suite feature validation is recorded in regeneration-validation.md.
- Staged source inventory excludes build/deployment output, sibling SDKs, and local
  reference artwork/working projects. Small tests/fixtures remain versioned.
- Initial whitespace checking reports inherited whitespace in source, licenses,
  archival documentation and the DXF template; these are retained in the baseline.
  Edited requirements and Help whitespace was cleaned.

The local repository starts on main. Author identity is configured locally from
the authenticated GitHub account, using its GitHub noreply address. This does not
change global Git configuration. Linux/macOS and Release were not tested.
