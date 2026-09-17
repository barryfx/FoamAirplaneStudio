# Drawn-up profile alignment and outline guides
Date: 2026-09-16

GentleLady's front profile had a slightly higher right corner; its following
profile had a higher left corner. The former highest-vertex seam matched those
opposite corners, twisting the forward fuselage. Matching top/right/bottom/left
centerline landmarks removes that ambiguity and preserves drawn up.

Top/Side boundaries also guide additional longitudinal sections. Refinement checks
every sampled guide breakpoint, retaining shoulders that lie between the original
uniform sections, with a 0.1 mm sampled-rail interpolation deviation limit.

Debug build passed. Focused CTest: 2/2 passed in 99.87 seconds:

- fuselage_profile_tests: 55.63 s
- fuselage_orientation_tests: 44.22 s

The orientation regression compares opposite roof slopes, reversed/reordered
curves and solid volume to catch a twisted/narrowed middle. A 0.2 mm-wide guide
shoulder between uniform sections verifies the resulting solid reaches the guide.
Existing profile workflow, project persistence, pointed ends and background
generation checks also passed. No Wing-specific tests were run.

Local evidence: `build/fuselage-orientation-build.log`,
`build/fuselage-orientation-tests.log`, and `build/gentlelady-guided.log`.
GentleLady is read directly from its ignored user project; no test rewrites it.
Its SHA-256 before testing was
`9C3C7B01E53B7CF274FD5292D315AA74F6A1DD1143EC245C7CD43724AC99D67C`.

The actual GentleLady guided solid completed successfully. Inspected the native
OCCT capture `build/debug/gentlelady-guided.png`: the front sections remain upright,
and the rounded nose and aft outline detail follow the guide contours. The user
project hash remained identical after validation. `git diff --check` passed;
the rebuilt Debug application was relaunched successfully.
