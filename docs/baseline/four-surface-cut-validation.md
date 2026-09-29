# Four-surface cuts and panel updates - 2026-09-26

## Changes

Fuselage Outline switches directly between Top and Side. New projects use Nose
Open and Tail Closed; stored choices are retained. Cut offers Top/Bottom/Left/
Right. Paths reaching the outline cut both opposite walls; wholly interior paths
stop at the inner cavity and cut only the selected wall. All detached solids are
retained as components, with no material removed. Format 31 migrates Side cuts
and active drafts to Left. Reference now explains plan tracing and wingspan
scaling, labels the physical-size choice Use Reference Image Scale, and Weight
and Balance reports Center of Gravity.

## Focused validation

The four-surface geometry test passed. For each surface, a closed interior cut
retains a 240 mm3 hatch with its centroid at the expected wall. It checks valid
solids, two separate components and volume conservation. A boundary-crossing
cut from each surface also splits the complete shell and conserves volume.
The Outline/Weight and Balance GUI checks passed, including default end states,
direct view switching, persisted choices and the renamed balance label.

Full Debug build and regression evidence is stored under build/debug:
four-surface-all-build-final.log, four-surface-regressions.log, and
four-surface-regression-results.xml. Generation tests are included at the user's
explicit request. No user project is modified by these regression tests.

## Final results

The full Debug run completed all 59 registered tests in 2321.77 seconds:
57 passed, one intentionally skipped legacy geometry placeholder, and one
failure in the holes test's synthetic version-24 persistence fixture. That
fixture had retained format-31's four cut layers. It now uses the historical
two-layer encoding, matching actual legacy files. The corrected holes test
passed both directly and through CTest's failed-test rerun (9.31 seconds).
Combined final coverage is 58 passing tests and one intentional skip, with no
unresolved failures. The rerun evidence is four-surface-regression-retest.log
and four-surface-regression-retest.xml.

The latest application and affected test targets rebuilt successfully; see
four-surface-final-app-build.log. The final Cut GUI test passed and its captured
panel was visually checked: all four surface buttons, retained-component
instructions, and status hint fit. Evidence is four-surface-cut-gui-final.log
and four-surface-cut-panel.png. The rebuilt Debug application launched, reached
input-idle, and remained responsive after its splash screen (process 33964).
