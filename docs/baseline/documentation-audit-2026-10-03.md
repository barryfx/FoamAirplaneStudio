# Documentation and local Git audit — October 3, 2026

Source revision: `adb5e9c` on `main`, before these documentation corrections.

## Scope and findings

Read `AGENTS.md` and all 180 existing files under `docs/`: the guide,
requirements, GUI design, 35 architecture documents, two format documents,
49 ADRs and 91 baseline/provenance files. Cross-checked current descriptions
against the project codec, GUI panels and workspace dispatch, resource manifest,
build presets, packaging scripts, website source and Pages workflow. Also checked
the root README, test guidance and the relevant bundled help descriptions.

Corrected stale current descriptions of:

- Format 31, supported reads through 31, four cut layers, end choices, circular
  profiles, component names, Assembly rotation and carbon-fiber/statistics fields.
  The optional statistics cache was introduced in format 29, not 30.
- Direct Top/Side outline switching, explicit end controls and default closure,
  mirrored main-half construction, retained single-wall hatches and through-cuts.
- Former SVG export and initially selected parts, Inspect/Weight and Balance
  toolbar order, carbon-fiber mass, the CG marker and statistics exclusions.
- The Use Reference Image Scale label, the separate CG resource image, later
  Ubuntu reference-test evidence and CMake 3.25 for schema-6 Linux presets.
- The distinction between implemented exports and planned servo-tray DXF/SVG or
  arbitrary additive fuselage regions.

Historical ADR text, dated validation results, archived DesignRC documents and
the original source hashes are retained as evidence of their recorded revisions.
Their earlier feature/version/platform claims are not current implementation
claims. The website and installer descriptions match their checked-in sources
and dated evidence; this audit does not revalidate live hosting or release assets.

## Validation

The existing Windows Debug build passed all five selected CTest entries in
26.71 seconds: `airplane_statistics_tests`, `export_tests`,
`project_persistence_ui_tests`, `fuselage_outline_tests`, and `reference_tests`.
The selected entry points avoid Wing/Fuselage generation; Export uses synthetic
cached shapes. The run is recorded locally in `build/debug/Testing/Temporary/LastTest.log`.

Strict UTF-8 decoding, current relative Markdown file targets, and Git whitespace
checks passed after the corrections. The archived DesignRC README retains its
two original license/notice links relative to the old project layout. No new
geometry, performance, installer or platform validation is claimed. This is a
documentation-only change, so the application was not rebuilt or relaunched.
The bundled help correction reaches deployed builds on the next build/package.

## Local repository verification

The starting worktree and index were clean, with no untracked nonignored files,
no stashes and no assume-unchanged or skip-worktree entries. Every existing docs
file was tracked. All local branches were merged into `main`; its starting tip
matched the locally recorded `origin/main`. `git fsck --full --no-dangling`
completed successfully without integrity errors.

The only ignored top-level collections were `build/`, `dist/`, and
`reference_imgaes/`: generated artifacts and local artwork/working projects remain
outside Git intentionally. They are not covered by a clean-worktree assertion.
These documentation corrections and this audit are committed together locally.
No remote fetch, push, history rewrite or ignored-file removal is part of the audit.
