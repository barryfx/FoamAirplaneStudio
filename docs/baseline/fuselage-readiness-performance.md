# Fuselage readiness and GentleLady performance
Date: 2026-09-17

## Behavior
A complete pair of outlines plus assigned closed station profiles suffices for
3D generation. Missing station walls initialize from the established LE defaults;
explicit values remain unchanged. Thicken, Cut, Servo Tray and Formers need not
be visited. Cut/tray/former features remain optional when no geometry is saved.
Both stabilizer workspaces enable on definition readiness and disable again when
a required profile is opened. Their modeling tools remain future implementation.

## Benchmark
Windows MSVC Debug, same host and toolchain, one sequential run per version with
no concurrent build/test jobs. Both use the saved GentleLady project, including
8/8/5 mm station walls, tray, three formers and one Side View cut. Input SHA256:
`9593735AD6E43E4E8D5630AC3789FD1175AC5C9B616C7F20B7E9ADD39F27CD6B`.
The user file was not modified; a frozen copy is `build/gentlelady-benchmark.foam`.

| Measurement | Baseline | Optimized |
| --- | ---: | ---: |
| Build including mesh | 562.595 s | 309.675 s |
| Tray supports | 171.464 s | 59.431 s |
| Final faces | 9564 | 7965 |
| Solid count | 6 | 6 |
| Volume (mm3) | 879734.2766335349 | 879734.2766333483 |

Elapsed build time decreased 44.96% (1.82x speedup) for this measured fixture.
These single-run Debug figures are evidence for this change, not a universal
performance claim. Timed progress callbacks record stages; post-build parity
checking and BREP output are excluded from build time.

The change merges coincident faces/collinear edges on outer and cavity lofts with
OCCT ShapeUpgrade_UnifySameDomain at 1e-7 linear tolerance, safe-input mode and
no spline concatenation. It preserves profile/guide samples, wall law, loft and
mesh settings, Boolean operations, cancellation boundaries and validity checks.
The unification call itself cannot be interrupted; cancellation is checked before
and after. No approximate section decimation or weakened validation was introduced.

Parity checks passed OCCT validity for all six solids, relative volume tolerance
1e-6, bounds within 1e-5 mm, and 429 ray intersection comparisons on three axes
within 1e-5 mm. Actual volume difference is approximately 1.87e-7 mm3 and reported
bounds match. This samples surface parity rather than proving every surface point.

Evidence (ignored local artifacts):
- `build/gentlelady-fuselage-baseline.log` and `.brep`
- `build/gentlelady-fuselage-optimized.log` and `.brep`
- `build/debug/Debug/fuselage_benchmark_baseline.exe`
- `build/fuselage-optimized-build.log`
- `build/fuselage-readiness-tests.log`
- `build/debug/fuselage-ready.png`

Reproduce with `fuselage_benchmark PROJECT.foam`. Optional `FOAM_BENCH_BREP`
selects the output shape; `FOAM_BENCH_REFERENCE` enables saved-BREP parity checks.
No Wing-specific tests or Wing model generation were run.

## Regression and visual checks
The Debug application and focused targets rebuilt successfully. CTest passed 4/4
in 38.79 s: readiness 5.77 s, former geometry 0.66 s, Cut 7.18 s, and wall geometry
25.12 s. Wall tests cover varying thickness and all open/closed nose/tail combinations.
The toolbar screenshot confirms both stabilizer actions are enabled. The optimized
GentleLady BREP was displayed in the actual OCCT viewport and visually inspected
(`build/debug/gentlelady-optimized.png`; `build/gentlelady-visual-smoke.log`).
The final Debug rebuild passed and the rebuilt application was launched.
GentleLady's SHA256 still matches the input recorded above.
