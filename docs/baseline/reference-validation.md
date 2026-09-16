# Reference implementation validation — 2026-09-13

- Windows Debug app build/deployment succeeded in build/debug and was launched.
- Inspected the Reference controls and a PDF plan displayed in the 2D viewport.
  The running user session and loaded reference were preserved.
- After adding test/deployment coverage, an attempted in-place relink was blocked
  by the running executable. A fresh complete Debug configure/build in
  build/reference-validation-debug passed, including all test targets and Qt PDF
  runtime/SBOM deployment, without closing the user's app.
- Added ReferenceTests.cpp coverage for PNG/JPEG physical metadata, PDF page size,
  load failure, project unit conversion, empty dimension values, and reset.
- Updated the viewport regression for scene bounds that include background items.
- No automated tests were run, following the user's existing instruction.
- Inherited OCCT deprecation and windeployqt VCINSTALLDIR warnings remain.
- Linux/macOS builds and additional interactive image-format checks were not run.
- Qt PDF 6.11.1 was installed from the official matching add-on; no existing Qt
  SDK files were replaced. No work occurred in DesignRC.
