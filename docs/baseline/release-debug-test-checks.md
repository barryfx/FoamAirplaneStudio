# Release and Debug regression check corrections

Date: 2026-09-25

The first full Release run reported 50 passes, four failures and one placeholder
skip. Investigation found test-side causes: standard assert removed setup/checks
under NDEBUG, an old off-center fuselage bound expectation, and an Inspect cache
fixture missing wing-root stations needed for the Assembly rotation pivot.

## Changes

- Added TestCheck.h with an always-evaluated TEST_CHECK macro and file/line failure
  diagnostics. It exits nonzero safely from ordinary code and Qt event callbacks.
- Converted 11 test files to that helper. Four had already supplied local,
  always-active replacements for assert; their duplicate macros were removed.
  The other seven now retain their standard-assert checks and setup in Release.
- Added two helper tests with NDEBUG explicitly defined: one verifies successful,
  single evaluation including side effects; one must exit unsuccessfully and is
  registered with CTest WILL_FAIL. Both are reported as passing when correct.
- Updated the narrow-shoulder orientation check to require Y bounds of -10 and
  +10 mm, retaining the same 1e-5 tolerance and matching ADR-0045.
- Supplied a valid open wing outline and unassigned root/tip stations before
  injecting the Inspect test's synthetic cache. Unassigned airfoils avoid
  accidental generation during that portion of the test.

## Validation

Built the complete Windows Release and Debug targets. The user explicitly
requested both complete regression suites, including geometry generation.
The suites run sequentially to avoid GUI interference.

Release: 56 passed, zero failed, one intentionally skipped placeholder;
57 registered tests total, 319.73 seconds.
Debug: 56 passed, zero failed, one intentionally skipped placeholder;
57 registered tests total, 1746.02 seconds.

All four previously failing tests pass in both configurations. Whitespace checks
passed, and no standard assert calls remain in the regression test sources.
The rebuilt Debug application was launched after both suites completed.

Evidence:
- build/release/corrected-tests-build.log
- build/release/corrected-regression-tests.log
- build/release/corrected-regression-results.xml
- build/debug/corrected-tests-build.log
- build/debug/corrected-regression-tests.log
- build/debug/corrected-regression-results.xml

The skipped designrc_geometry_tests executable is a pre-existing placeholder
returning CTest's configured skip code. Active wing, fuselage, stabilizer and
other generation regressions remain included. Application behavior and geometry
implementation are unchanged by these test corrections.
