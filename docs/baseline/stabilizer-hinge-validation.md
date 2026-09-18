# Stabilizer hinge validation

2026-09-18, Windows MSVC Debug. Built designrc, stabilizer_outline_tests and
sketch_editor_tests with windows-debug. Final CTest run: stabilizer_outline_tests
passed in 84.63 s; sketch_editor_tests passed in 1.25 s (85.93 s total).

Coverage includes:
- Known-thickness box separation with Tape/Standard profiles. Solid classification
  checks the 45-degree relief and confirms the shorter return segment stays square.
- Reversed segment order/direction preserves cut volume; a path ending inside the
  body is rejected. Generated cut bodies pass OCCT validation.
- Both UI panels: exclusive radio choices, connected line drawing, drawing-button
  cancellation, Escape, segment deletion, and isolation from outline editing.
- Save/Open retains an unfinished connected-line draft and Standard choice.
  Version-18 migration initializes empty Tape hinges; malformed cut enums/counts
  are rejected. Existing project, endpoint and airfoil tests still pass.
- Full stabilizer worker generation with the new cut produces four horizontal
  bodies and two vertical bodies. Existing cancellation/processing checks pass.
- Existing sketch editor checks pass with continuous-line mode opt-in only.

Both panel and cut-model captures were visually inspected under
build/debug/stabilizer-hinge-final-*.png (generated, not source artifacts).
The running workspace application was stopped before rebuilding and the rebuilt
Debug application relaunched. No Wing or Fuselage generation tests were run.
Linux/macOS were not tested. Curved hinge heights use the existing sampled
construction; no machining-tolerance claim is made.


## Joined horizontal bodies and persistent Cancel (2026-09-18)

Horizontal generation now fuses matching mirrored bodies individually: one solid
without a hinge cut, two solids with a cut. Final stabilizer-only suite passed in
149.29 s. Checks verify OCCT validity, unchanged volume relative to two half-models,
and that each joined fixed/elevator solid spans both sides of the centerline.
Both horizontal and vertical UI generation report two bodies after a hinge cut.

The shared bottom-panel Cancel control is visible but disabled while idle in both
stabilizer workspaces. Tests verify its parent/placement, enabled processing state,
actual cancellation, and disabled state after completion. Active and idle captures
under build/debug/stabilizer-joined-* were visually inspected. Debug was rebuilt
and relaunched. No Wing or Fuselage generation tests were run.
