# Fuselage alignment-pin validation
Date: 2026-09-18
Branch: codex/fuselage-alignment-pins

## Scope
Four pins on the negative-Y main-body half, matching sockets on positive Y;
cut-out pieces and removable tray/formers remain separate and unchanged. Added
focused alignment coverage to FuselageCutTests and a dedicated CTest entry.
The existing profile volume assertion now accounts for four 0.5 mm socket-bottom
clearances, instead of expecting exact total material conservation after pins.
Pre-feature split conservation remains checked in production.

## Checks
Debug application and focused targets compiled successfully on Windows/MSVC,
Qt 6.11.1 and OCCT 8. Existing nonfatal Qt deployment warnings remain.
The initial focused alignment test passed in 2.10 s. Subsequent five-case
regeneration checks passed in 36.94 s, covering former insertion/defaults,
optional-tab readiness, cuts and alignment geometry. Further final checks after
sloped-skin placement refinement are recorded below.

Numeric assertions cover four pin locations; 3 mm projection and 3.5 mm socket
depth; diameter 4 mm in a 5 mm wall and 2 mm in a 2 mm wall; smoothly varying
station thickness; positive-volume, valid halves; no interpenetration; exact
material delta for socket bottom clearance; hatch identity preservation;
insufficient transverse material rejection; and cancellation.

Fuselage tests are authorized by the user's earlier explicit regeneration-test
request. Wing generation tests remain excluded. The full GentleLady benchmark
has not been repeated for this feature; earlier rail timings predate pins.
No persistent fields or project format changes are introduced by alignment pins.
The pending Stabilizer work was preserved.

## Final verification
After the sloped-skin refinement, Debug rebuild passed and six focused CTest
cases passed in 200.47 s:

| Test | Seconds |
| --- | ---: |
| fuselage_profile_tests | 162.02 |
| former_tests | 8.81 |
| fuselage_readiness_tests | 6.00 |
| former_defaults_tests | 13.34 |
| fuselage_cut_tests | 7.88 |
| fuselage_alignment_tests | 2.36 |

The profile suite covers rectangular-to-curved sections, pointed ends, solid
volume, background-job completion and cache reuse with zero Wing model revisions.
The alignment fixture was meshed and exported with halves translated apart solely
for visual inspection. OCCT viewport capture was reviewed: the mating surfaces
show cylindrical pins and matching blind sockets. Numeric tests verify all four
features, including portions hidden from this isometric camera.

Artifacts: `build/debug/fuselage-alignment-exploded.brep` and
`build/debug/fuselage-alignment-exploded.png` (ignored generated files).
Reproduce with `FOAM_ALIGNMENT_BREP` set to an output path and
`fuselage_cut_tests --alignment-only`; the existing `former_tests --preview-brep`
helper displays the saved fixture. `git diff --check` passed. Rebuilt Debug
application launched after testing. No Wing generation tests were run.

## Rear placement adjustment — 2026-09-20
Moved the preferred rear placement from 85% to 75% of main-body length; the
front target and safe-placement search ranges remain unchanged. The focused
regression checks both rear pins and sockets at 75%, absence at the old 85%
location, and the existing depth, diameter, thin/variable-wall and safety cases.
Debug application and test target rebuilt successfully. Only
`fuselage_alignment_tests` ran and passed in 2.28 s. Logs are
`build/rear-alignment-build.log` and `build/rear-alignment-tests.log`.
