# Focused reference/viewport tests — 2026-09-13

Updated and rebuilt the focused Debug test targets:
- reference_tests: PNG/JPEG physical metadata, PNG without density metadata,
  all-page PDF loading and combined page height, missing-file handling, project
  unit conversion, empty dimensions, and reset.
- designrc_gui_tests: document display/clear, stacked page placement, fit-width
  on load/resize, vertical scrollbar availability, wheel zoom in/out across
  scrollbar appearance, explicit scrolling without scale changes, and Fit View.

Initial CTest execution exposed a test configuration problem: Windows was forced
to use Qt's offscreen platform, which windeployqt does not deploy with this app.
The reference test passed when using the Windows backend. CMake now requests
offscreen only on non-Windows platforms.

Final command:
ctest --test-dir build/reference-validation-debug -C Debug -R "^(reference_tests|designrc_gui_tests)$" --output-on-failure --timeout 60

Result: 2/2 passed; reference_tests 0.21 s, designrc_gui_tests 0.34 s; total 0.57 s.
No unrelated geometry/domain/STEP suites were needed for this focused update.
Linux/macOS were not tested. The running application session was not modified.
