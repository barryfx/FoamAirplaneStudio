# Fuselage Cut validation
Date: 2026-09-17

Windows Debug application and Cut/Thicken test targets rebuilt successfully.
Focused CTest run passed 2/2 in 25.54 s:
- `fuselage_cut_tests`: 23.93 s.
- `fuselage_thicken_ui_tests`: 1.50 s.
No Wing-specific suites were run.

Geometry checks exercise independently scaled/translated Top and Side cut paths,
connected Line/Spline sheets, combined cuts yielding four solids, closed hatch
paths, a hollow-body split, positive solid volumes and material conservation.
Short interior paths and branching paths are rejected with descriptive errors.
Cancellation is checked before geometry work.

GUI tests draw joined Line/Spline segments by mouse, move a shared sketch point,
select/Delete a segment, switch views, and save/reopen a pending spline. They
verify version-13 persistence, version-12 migration and malformed cut rejection.
A pixel check confirms paths remain visible outside Cut. The Cut panel screenshot
was visually inspected. The actual Fuselage worker builds and displays two bodies;
its model revision is reused on unchanged 3D navigation, while Wing revision stays
zero. Solid-count and validity checks establish separate OCCT bodies, although
they remain in the same display location without exploded-view controls.

Evidence (ignored local artifacts):
- `build/fuselage-cut-build.log`
- `build/fuselage-cut-tests-build.log`
- `build/fuselage-cut-tests.log`
- `build/debug/fuselage-cut.png`

These checks do not certify arbitrary self-intersecting or highly complex paths.
Cuts have zero kerf and preserve all resulting material; clearance editing and
per-body placement/export workflows are outside this implementation.

The existing Fuselage regression checks also passed: `fuselage_outline_tests`
(2.11 s) and `fuselage_profile_tests` (40.21 s), 2/2 in 42.33 s. Log:
`build/fuselage-cut-regression.log`. The final application rebuild succeeded and
the rebuilt Debug executable was launched. GentleLady's file checksum remains
unchanged; no user project was overwritten during validation.
