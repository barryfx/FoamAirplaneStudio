# Stabilizer airfoil and model validation
Date: 2026-09-17

Windows MSVC Debug / Qt 6.11.1 / OCCT 8. Debug application and focused stabilizer
test target rebuilt successfully. The existing VCINSTALLDIR deployment warning
remains nonfatal. Final stabilizer_outline_tests passed in 24.91 seconds (24.97
seconds total CTest). No Wing or Fuselage generation tests were run.

Geometry checks call only StabilizerSolidBuilder. They verify valid positive-volume
single fins, mirrored horizontal pairs, 90-degree rotated outlines, uniform scaling,
mirror bounds, finite-chord tips and a pre-cancelled processing token. Adaptive OCCT
volume integration is used for the scale comparison; the non-adaptive default was
not accurate enough for the 0.01% comparison tolerance on these spline surfaces.

UI/lifecycle checks cover both stabilizers:

- Only Outline selectable initially and after deleting the outline; valid outlines
  enable the remaining actions.
- Exact bundled NACA009 default and DAT header name, available before Airfoil is
  visited; initial 3D generation uses that default.
- A background job activates the common processing lock and Cancel button. Cancel
  discards the result; re-entering 3D retries successfully. Bodies are two horizontal
  halves or one vertical fin. Unchanged 2D/3D navigation reuses the cached model.
- Custom airfoils replace only their component selection. Failed loads retain the
  selected profile. Save/Open restores the name and coordinates after deleting the
  source DAT, and the restored custom airfoil generates a valid model.
- Version-16 migration supplies defaults. Malformed array lengths and zero-thickness
  airfoils are rejected, alongside existing invalid outline/draft cases.
- Existing outline drawing, editing, warning, remapping and lifecycle tests pass.

Both horizontal and vertical 3D previews and the Airfoil panel were visually
inspected. Generated captures are under build/debug/stabilizer-model-* and are not
source artifacts. The horizontal preview shows the mirrored pair; the vertical
preview shows one upright fin. The Airfoil panel shows instructions, load button,
and the trimmed name from the supplied DAT in the requested order.

The running workspace application was stopped before rebuilding. The rebuilt
normal Debug application was launched. Linux/macOS were not tested. Hinge Line,
Cut and assembly placement remain future work. Manual stabilizer dimensions assume
the same drawing scale as the Fuselage Side View; see architecture/stabilizer-solids.md.


## Explicit leading-edge selection (2026-09-17)

Debug `stabilizer_outline_tests` passed in 39.11 seconds and
`sketch_editor_tests` passed in 1.27 seconds. Both Outline panels were visually
checked for the exact Select Leading Edge End Point button and green LE marker.
Coverage includes missing/interior selections, Escape, either endpoint, toolbar
gating, dirty-state changes, Save/Open, version-17 migration, malformed saved
indices, deletion/remapping, scaling, and generation with a reversed LE choice.
An asymmetric swept planform checks that the selected endpoint reverses the
builder's chord direction. Existing stabilizer generation, cancellation and
cache checks pass. No Wing or Fuselage generation tests were run.

The workspace Debug application was closed before rebuilding and the rebuilt
application relaunched. Version 18 saves the endpoint choice; pre-18 projects
open with no choice and require the user to select it. Linux/macOS not tested.

## Main-branch integration check (2026-09-18)
Rebuilt the Debug application, stabilizer_outline_tests and sketch_editor_tests
before committing the combined baseline. Focused CTest passed both suites:
stabilizer_outline_tests 25.78 s, sketch_editor_tests 1.23 s (27.09 s total).
No Wing generation tests were run. Separately authorized Fuselage regeneration
and rail-optimization evidence is in former-retainers-validation.md. Current
README, architecture summaries and bundled Help now describe project format 18,
implemented Stabilizer modeling, and the fuselage retaining rails/centre split.
