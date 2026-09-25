# Fuselage cancellation validation

Date: 2026-09-23. Branch: `codex/fuselage-speedup`.

## Failure and correction

Cancel already reached the worker stop source and bounded child workers. Pin
searches nevertheless ran whole-shape ray loading/intersections, classification,
and mass integration without intermediate stop checks. On the frozen BabyBuzzard
fixture, a request one second into the pin-search stage took 14.8372 seconds to
be acknowledged before this correction.

An intermediate checkpoint-only implementation improved early cancellation to
6.45625 seconds, but a request 60 seconds into the search still took 164.848 seconds
(and 94.1508 seconds on a diagnostic repeat). Native stack sampling showed worker
exception unwinding inside `BRepClass3d_SolidExplorer::Destroy`, freeing thousands
of eager face intersectors and contending in the Debug CRT heap. The stop had been
accepted, but joining workers waited on this costly cleanup.

The final implementation removes those duplicate whole-solid classifiers. It
keeps oriented analytic ray crossings per solid to derive material intervals and
loads face intersectors lazily after an expanded X/Y bounds check. Tangencies and
coincident edge hits are grouped per solid. This remains a rejection filter:
exact cylinder/stock intersection volume still certifies each accepted pin.
Each worker owns its CAD copies and bounded-to-visited-faces cache.

Fuselage bounds and analytic volume integration check between faces across
hollowing, formers, supports, cuts, holes and alignment features. Volume integrals
retain one common reference point and existing face orientations; no sampling
substitute is used. Wall offsets pass the child stop token through their numeric
fallback. Reflection and validity checks check cancellation before and after
kernel calls. Loft, Boolean, sewing and mesh calls retain their OCCT progress
indicators.

Cancellation remains cooperative. Individual OCCT calls without a stop API must
return before their next checkpoint; there is no claim of instantaneous stopping
inside every kernel call, no terminated threads, and no detached geometry workers.
A cancelled result is never published, and child workers join before editing resumes.

## Validation

Local logs under `build/debug/`: `fuselage-cancel-before.log`,
`fuselage-cancel-after.log`, `fuselage-cancel-tests.log`, and
`fuselage-cancel-build.log`. No Wing generation tests are included.

Processing tests compare oriented ray intervals to OCCT solid classification on
solid, through-hole and hollow shapes, and verify roof/floor intervals on the
shared seam of two hollow halves. They also compare analytic mass/centroid and
bounds with the original OCCT routines (including translated geometry), and
cancel an active many-face integration. The existing alignment tests verify
pin/socket geometry. The GUI test clicks Cancel with four active child workers,
checks the Cancelling state, then verifies all workers joined, no result was
published, controls were restored and project data stayed unchanged.

## Final real-project result

With the final lazy ray cache, requesting cancellation 60 seconds into the same
BabyBuzzard pin-search stage was acknowledged in **13.5333 seconds**, with no
result published. Log: `build/debug/fuselage-cancel-final-search.log`. This is a
single observed stop latency, not a guaranteed upper bound. Native kernel calls
and releasing the private CAD copies still take time; the multi-minute eager
classifier destruction has been removed. No changes were made to the project.
The diagnostic stacks are in `build/debug/cancel-active-stacks.log`.

The final `processing_tests` and `fuselage_alignment_tests` both passed (1.91 seconds
combined). The former includes ray/classifier parity, hollow-half seam behavior,
mass/bounds parity and active cancellation; the latter checks actual pin/socket
geometry. Log: `build/debug/fuselage-cancel-lazy-tests.log`.

The final additional four suites all passed in 364.48 seconds:
`fuselage_cancellation_tests` (every reported stage), `view_controls_tests`,
`fuselage_wall_offset_tests`, and the seven-case `fuselage_symmetry_tests`.
Together with the two suites above, six focused suites passed. Log:
`build/debug/fuselage-cancel-final-tests.log`. The final Debug app built
successfully and was relaunched after validation.
