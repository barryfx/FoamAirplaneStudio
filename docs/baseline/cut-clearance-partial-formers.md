# Single-wall cut clearance and partial formers — 2026-09-26

Single-wall Cut generation now checks the full closed hatch footprint against
the inner cavity before splitting. A near-to-far connection rejects interference
with a parallel wall, including an obstruction wholly enclosed by the loop.
The error names the surface and cut and instructs the user to move or shrink it.
The existing worker failure handler stops processing and displays the error.

Former cavity intersection already supports masks crossing only the upper or
lower Side View outline. The panel now explicitly explains this behavior.
New geometry checks verify both cases stop at the opposite rectangle edge,
remain clear of the skin, and fit their retaining rails. GUI checks resize
rectangles into each configuration. No former fitting algorithm change was
needed, and no project format change was made.

Debug-only validation:

- `former_tests`: passed (18.82 s), including GUI and model generation.
- `former_geometry_tests`: passed (6.44 s), including both partial-height cases.
- `fuselage_surface_cut_tests`: passed (1.20 s), including interference near
  both transverse edges for all four surfaces and an enclosed parallel bridge.
- `fuselage_cut_tests`: passed on final rerun (13.27 s), including regeneration
  stopping, model-unready state, and the surface/path-specific status error.

The first GUI run was stopped because its temporary project needed saving after
generation initialized defaults. The fixture now saves before reopening. A
subsequent test cut was clear of the wall after project scaling; moving it to
0.05 drawing units inside the outline created the intended interference.
No production changes were required for these fixture corrections.

Build/test evidence: `build/debug/cut-clearance-formers-build.log`,
`cut-clearance-formers-tests.log`, `cut-clearance-gui-rebuild.log`, and
`cut-clearance-gui-retest.log` (with corresponding test XML). The application
and affected tests were built using the Windows Debug preset only.
