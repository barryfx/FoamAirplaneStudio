# BabyBuzzard36 top-hole clearance investigation — 2026-09-24

The original project and the earlier read-only rail-test snapshot have identical
SHA-256 hashes. The isolated Top hole 1 prism was tested against the previously
captured inner cavity. At fuzzy tolerances 1e-7, 1e-6, 1e-5 and 1e-4 mm, each cut
reported success and valid topology but retained a connected path from the top
to the bottom. Unlike the rail regression, a bounded tolerance increase did not
resolve this case.

An independent grid sampled 1,117 points inside the hole footprint. Eight vertical
rays missed the inner cavity near the forward lateral edges (model X approximately
92.9–108.1 mm, Y approximately -17.2 or +17.9 mm). This is a real footprint/cavity
clearance problem, rather than evidence that a tighter tolerance rejected a
correctly isolated cutter. The model scales while physical wall thickness stays
fixed.

The fresh full-project reproduction reached the hole stage at 674.845 seconds
and failed on Top hole 1. Repeating the ray check against that exact newly
captured cavity and prism reproduced the same 8 misses out of 1,117 samples.

With the user's authorization, a temporary copy in
`build/debug/BabyBuzzard36-smaller-top-hole.foam` narrows only Top hole 1's lateral
coordinates by 15% about their bounding-box center; its length and the rest of
the project are unchanged. This isolated cutter passes wall isolation at the
original tolerance. All 1,119 sampled rays in the narrowed footprint hit the
cavity. The original project remains untouched.

Production retains the existing tolerances and opposite-wall protection. The
isolation diagnostic now names the wall and hole number. On failure, an independent
boundary-ray check confirms parallel-wall interference only if a ray crosses
fuselage material without intersecting the cavity. Otherwise the error preserves
the possibility of a numerical CAD failure. Both paths stop generation.

The focused hole suite passed after the change: four wall directions,
opposite-wall preservation, multiple paths, splines, invalid geometry,
cancellation, editor controls, persistence and a generated fuselage. Added
regressions cover valid and invalid second holes on all four walls at scales
0.5, 1 and 2 through the production drawing-to-model projections, with fixed
physical wall thickness, including diagnostic identity.
Additional cases exercise longitudinal side walls and roof/floor walls as well
as the end wall, at all three scales and all four cut directions.

`former_tests --project-holes <project> <body.brep> <cavity.brep> interference|success`
replays the real project hole stage against captured pre-hole operands without
regenerating the loft or rails. It checks the original's specific interference
diagnostic or the corrected copy's valid topology, solid count, removed volume
and unchanged input body.

Evidence is in ignored `build/debug/hole-*.log` files. Temporary CAD capture and
investigation code is not part of the production change.

Final validation:

- The expanded `former_tests --holes` suite passed, including the parallel-wall
  diagnostics, GUI/persistence checks and synthetic fuselage generation.
- Replaying the exact original operands produced the expected
  `Top hole 1 overlaps a wall parallel to the cut direction` diagnostic.
- Replaying the narrowed copy passed validity, unchanged solid count and
  unchanged input checks; the hole removed approximately 21,723.1 mm3.
- Full fuselage generation of the narrowed temporary copy passed through holes,
  the Side View cut, four alignment pins/sockets and meshing. Build time was
  1,344.95 seconds; final topology validation passed with seven solids and total
  volume 678,923.0670104817 mm3. This covers the fuselage pipeline, not Assembly
  or Wing generation. No changes were made to the user's original project.

Final logs: `hole-wall-tests.log`, `hole-wall-original-project.log`,
`hole-wall-smaller-project.log`, and `hole-smaller-full-project.log`.
After testing completed, the Debug application rebuilt successfully and was
launched. The original project's SHA-256 remained unchanged.
