# Former retainers and main-body halves validation
Date: 2026-09-18
Branch: codex/former-retainers-and-fuselage-halves

- Debug build passed: `cmake --build --preset windows-debug --parallel 4 --target designrc former_tests`.
- `build/debug/Debug/former_tests.exe --editor-only` passed: units, add/move/resize,
  overlap rejection, delete, persistence, overlay visibility and instructions.
  The editor-only path exits before entering 3D and asserts zero Wing/Fuselage
  model revisions. No geometry-only, defaults-only or readiness-only paths ran.
- Reviewed `build/debug/former-retainers-editor.png`: retaining rail instructions,
  Width, Add Former and Delete Former controls are visible without clipping.
- `git diff --check` passed.
- Rebuilt Debug application launched without opening a project for generation.
- At initial implementation, Wing and Fuselage generation tests were deferred
  under AGENTS.md. The user subsequently explicitly requested Fuselage
  regeneration testing; results are recorded below. Wing generation remains excluded.
- Existing future full-generation UI assertions were adjusted for one additional
  main-body solid; these were exercised in the subsequent authorized checks.

Known policy: the largest post-cut solid is the main body; equal-largest solids
are rejected. Smaller cut-out pieces and removable inserts remain unsplit. No
persistent format change is needed. Pre-existing Stabilizer changes were kept.

## User-authorized regeneration checks (2026-09-18)
- Rebuilt `fuselage_benchmark`, `former_tests`, `fuselage_cut_tests` and
  `servo_tray_tests` successfully.
- Six focused CTest cases passed: `former_tests`, `fuselage_readiness_tests`,
  `former_defaults_tests`, `former_geometry_tests`, `servo_tray_tests`,
  `fuselage_cut_tests`.
- Added and executed numeric rail checks: four 4 x 3 x 30 mm bands on a known
  cavity add exactly 1,440 mm3, occupy both sides at multiple heights, remain
  within fore/aft and inward limits, and do not overlap the former insert.
- Centre splitting produces two halves and conserves material volume.
- A cut-out crossing Y=0 remains the identical OCCT solid after main-body
  splitting, with three total bodies and unchanged total volume.
- UI regeneration checks include unedited defaults, optional-tab readiness,
  former/tray counts and cached Fuselage reuse. Wing revision remains zero.

GentleLady baseline regeneration completed successfully: 7 valid solids,
1,226.37 seconds including mesh, volume 891,302.66251403 mm3. Rail generation
ran from 225.252 s to 1,082.16 s (856.908 s). The unchanged input SHA256 is
`64F5328181B58C47EA799613C474F775B4F469A29DAA0333213AB4C239A1241B`.
The frozen fixture and baseline BREP/log are under `build/debug/`.

## Rail-generation optimization
The user subsequently requested investigation of rail-generation speed. Rails now
skip exact insert cuts only for conservatively disjoint bounding boxes, and all
rails join the shell in one multi-tool fusion instead of up to twelve shell fusions.
Added per-former construction and final-fusion progress messages. Geometry
sampling, dimensions, fuzzy tolerance, safe-input mode and cancellation remain.

Windows MSVC Debug, same frozen GentleLady input and host, one run per version:

| Measurement | Original | Optimized |
| --- | ---: | ---: |
| Rail stage, including validation | 856.908 s | 416.100 s |
| Complete build including mesh | 1,226.370 s | 782.717 s |
| Valid solids | 7 | 7 |
| Faces | 8,958 | 8,958 |
| Volume (mm3) | 891,302.66251403 | 891,302.66251403 |

Measured rail-stage time decreased 51.44% (2.06x); total generation time decreased
36.18% (1.57x). Optimized rail construction was 180.016 s and the final fusion plus
validation 236.084 s. The latter remains the largest rail-stage cost.
These are single-run wall-clock observations, not a statistical benchmark. Brief
builds and focused test runs overlapped portions of the original run; the optimized
run had no concurrent build/test workload. Sampling and kernel settings were unchanged.

Full saved-BREP parity passed: valid solid count, volume within one part per
million, bounds within 0.00001 mm, and all 429 ray intersections across three axes
within 0.00001 mm. Seven solids correspond to two main halves, the cut-out, the
tray and three formers. BREP writing and parity checking are outside build timing.

The optimized code passed `former_tests`, `former_defaults_tests` and
`former_geometry_tests`. Added close-former and tray-clearance coverage passed:
intersecting rails fuse into one supported body without penetrating any insert.
Debug application rebuilt successfully; no Wing generation tests were run.

Artifacts (ignored build outputs):
- `build/debug/gentlelady-retainers-fixture.foam`
- `build/debug/gentlelady-retainers.log` and `gentlelady-retainers.brep`
- `build/debug/gentlelady-retainers-optimized.log` and `gentlelady-retainers-optimized.brep`
- `build/debug/Debug/fuselage_benchmark_rail_baseline.exe`

Reproduction: set `FOAM_BENCH_REFERENCE` to the original BREP and
`FOAM_BENCH_BREP` to a new output path, then run `fuselage_benchmark` with the
frozen fixture path. The project in `reference_imgaes` was not modified.

Visual smoke check: loaded the optimized BREP in the actual OCCT viewport;
the complete fuselage displays successfully (`build/debug/gentlelady-retainers-optimized.png`).
This assembled exterior view does not expose the internal rails; their validation
is provided by the numerical geometry tests and baseline parity above.
The rebuilt Debug application was launched after validation.
