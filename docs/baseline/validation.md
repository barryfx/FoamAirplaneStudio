# Baseline validation — 2026-09-13

- Windows Debug configure: passed (MSVC 19.50, Qt 6.11.1, sibling OCCT Debug).
- Windows Debug build with C++23: passed.
- Application startup smoke: opened with title FoamAirplaneStudio - Wing Baseline,
  remained running, and closed normally with exit code 0.
- designrc_domain_tests: passed (7.53 s).
- designrc_gui_tests: passed (19.72 s).
- designrc_step_export_tests: passed (8.46 s).
- designrc_joiner_backend_tests: passed (26.14 s).
- designrc_geometry_tests: stopped at the user's request after approximately
  152 seconds. CTest reports the terminated process as failed; this is an
  interrupted run, not a completed regression result. Last reported stage:
  multiple spars and sheeting. Do not claim geometry-suite success.

All copied test files/fixtures match the source manifest. Source DesignRC files
in the manifest have unchanged SHA-256 hashes, and its Git status remains clean.
Existing FoamAirplaneStudio AGENTS.md and requirements/GUI documents were not edited.
FoamAirplaneStudio has no Git repository, so no commit was made.

Build warnings include inherited OCCT deprecated API usage and windeployqt's
VCINSTALLDIR warning. The local Debug executable starts successfully.
Linux, macOS, Release builds, and full interactive rendering were not validated.
No renderer implementation was changed. No performance claim is made.
