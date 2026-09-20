# Former 3 rail clearance investigation

Date: 2026-09-20. Windows/MSVC Debug, branch Export.

The reported `reference_imgaes/Components.step` contains broad, unperforated
front-rail faces at X=441.84958 and X=445.84958 in both fuselage halves, reaching
the centreline. Export Former 3 spans X=445.84958 to 448.84958; it is the first
former in the saved GentleLady input order. Its rotation is zero.

A dedicated diagnostic captured the supported body, inner cavity, and four
removable inserts immediately before rail generation. It used GentleLady's
saved inputs without running a regression executable or regression suite.
Captured files and diagnostic sources/logs are in `build/rail-investigation/`.

The original front pocket volume is 7,257.077679007 mm3. Cutting a copy shifted
by +3 mm returned 7,257.077966780 mm3 in two topologically valid solids, with
the centre point inside retained material. The -3 mm cut returned
593.376110421 mm3 and an empty centre. Thus the defect occurs in rail subtraction,
before fusion, Assembly or STEP serialization; topology checks did not catch it.

Extending the clearance slice by 0.1 mm at both local-X ends removes the
coincident cap faces from the subtraction. The revised +3 mm result is
593.375932337 mm3; the -3 mm result is 593.376110430 mm3. Both leave the centre
empty. Only the cutter is extended: the rail remains 4 mm wide and projects
3 mm inward. The construction follows the actual cavity, including taper.

Production code also rejects residual solids whose mass centre is strictly
inside both that solid and the clearance tool. This detects an obvious cavity
plug; it is a sanity check, not an exhaustive proof of Boolean correctness.

Additional regression checks were coded for tapered-wall rail depth, volume,
and open centres at multiple heights for straight and rotated formers. They
were compiled, but not run, per the user's instruction. Debug `designrc` and
`former_tests` builds succeeded (`build/rail-fix-build.log`).

A targeted invocation of the revised production retainer function on the
captured supported body added the affected former's four rails and completed
wall fusion with valid topology. All eight centreline samples were outside the
resulting material: X=443.84958 and 450.84958, each at Z=-10, 0, 10 and 20 mm.
Evidence: `build/rail-investigation/fix.log` and `fixed-body-former3.brep`.

No full post-fix Assembly/STEP export or regression suite was run. The user's
existing STEP file was not overwritten. Regenerate the model before exporting
to obtain the corrected rails.

## Subsequent regression validation
The user subsequently authorized focused regression and fuselage generation
checks. `former_geometry_tests` passed in 4.95 s, covering the new tapered and
rotated rail assertions. The Holes/Cut fuselage checks also passed; see
`fuselage-holes-validation.md`. The original GentleLady STEP was not overwritten.
