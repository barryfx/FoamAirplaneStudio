# Documentation and source release audit — 2026-10-04

## Scope and corrections

Prepared source version 0.2.0 on `main`, following AGENTS.md. All existing local
branches were already merged into `main`. The existing implementation commits
include fiberglass, incremental mass caches, stiffeners, the fuselage support
checkpoint, entered distance units, bounded lofts and straight Side View routes.

Updated README, the documentation guide, in-app help and website feature text
to match those changes. Corrected the format specification's obsolete claim
that stiffener unit preferences were transient. Current project format remains
33; no schema change was made for this release. CMake and Windows packaging
defaults now use 0.2.0. Current build instructions and Linux script defaults use
eight build jobs. Historical commands and validation results retain their original
values; the preserved DesignRC provenance documents were not rewritten.

The website and README distinguish source 0.2.0 from the latest published 0.1.0
installers. Their three download URLs were checked against GitHub Release asset
metadata. No new installer packages or GitHub Release assets were produced.

The pre-existing change to the tracked BabyBuzzard example was preserved. It
uses format 33, fiberglass patches and one 3 × 1 mm Strip per side from 20% to
85%, with updated placements and display metadata. The embedded reference image
was unchanged. This updated example cannot be opened by 0.1.0.

## Validation

- Windows Debug configure and build of `designrc`, `length_display_tests` and
  `project_tests` passed with `--parallel 8`. OCCT deprecation warnings remain;
  the optional Vulkan headers were unavailable and did not prevent the build.
- `length_display_tests` passed, reading the updated bundled example and
  checking editor restore, unit metadata and encode/decode round trips.
- `project_tests` with `FOAM_PANEL_ONLY=1` passed its non-generation panel checks.
- The rebuilt Debug application launched and responded with the normal main window.
- Markdown relative-link checks, HTML internal-anchor checks, version consistency
  and `git diff --check` passed. The archived `DesignRC-README.md` retains two
  original sibling license links as provenance; these are not current guide links.

Generation suites were not rerun for these documentation/version changes.
The latest implementation evidence is [straight-side stiffener validation](straight-side-stiffener-validation.md),
including six relevant Debug tests, the final route rerun, captured-body cutting
and a Release build. That captured BabyBuzzard test used Stop 75%; it does not
establish complete generation of this revised Stop 85% bundled example.
Linux/macOS builds, new installer builds and browser visual inspection were not
performed in this audit. Website changes are text-only and retain the existing
responsive layout and wrapping links.
