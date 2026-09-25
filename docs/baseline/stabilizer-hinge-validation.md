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

## Error handling and vertical hinge selection (2026-09-23)

Incomplete Cut Shapes are rejected before workers start and before the builder
reports any geometry stage. Failed stabilizer jobs restore the UI and suppress
automatic retries until geometry changes or the user explicitly re-enters 3D.
Vertical hinges choose the segment closest to model span direction; horizontal
hinges retain the longest-segment rule. Other connected segments stay square.

Validation on Windows Debug:
- stabilizer_outline_tests passed in 264.57 seconds: early cut rejection, worker
  error recovery (controls, cursor, Cancel visibility, no successful model),
  two-segment hinge with a longer return, reversed traversal, existing geometry,
  editor, persistence, caching and cancellation coverage.
- processing_tests passed in 0.23 seconds, including sibling cancellation and
  joining after an indexed worker throws.
- --vertical-project on the current BabyBuzzard.foam completed successfully with
  its saved two-line hinge and Cut Shape, producing valid fixed/control bodies.
  The user's project was read only and not modified.
- Debug application rebuilt and launched. No Wing or Fuselage generation tests
  were run. Logs are under build/debug/stabilizer-failure-tests.log and
  build/debug/babybuzzard-fin-validation.log.

An older suite cleanup assertion expected unselected Delete to erase a complete
Cut Shape; it now uses the existing Delete Cut Shape button, consistent with
individual-curve Delete behavior.

## Curved-airfoil bevel regression (2026-09-23)

The earlier valid-solid check missed BabyBuzzard's absent rudder bevel. Direct
cross-section measurements reproduced full rudder thickness beside the hinge.
The generic ruled-loft tool was valid and covered the correct stock, but the
Boolean removed only about 1.3 mm3 from a 13490.6 mm3 control body. Reducing tool
bounds and using a fuzzy Boolean did not fix the missing relief.

Constructing the same sampled envelope from explicit planar faces, sewing the
shell and orienting its solid produced the intended subtraction (about 1394.5
mm3 in the diagnostic). Tool bounds now follow thickness. Split/cut errors and
invalid or unclosed tools abort generation. Temporary diagnostic exports/logging
were removed from source.

At the middle of BabyBuzzard's vertical Tape hinge, before the fix a point 0.1 mm
inside the rudder retained thickness from -2.52953 to +2.52953 mm. After the fix,
its lower surface is +2.43420 mm; at 1.0 mm inward it is +1.53420 mm. The change
is 0.9 mm over 0.9 mm, demonstrating the 45-degree face.

The new stabilizer_bevel_tests target contains an artwork-free BabyBuzzard
stabilizer fixture. It measures lower relief slopes and hinge contact for Tape,
both lower/upper slopes and center contact for Standard, at three stations per
component. It also checks both mirrored elevator halves and solid validity.
The executable accepts a .foam path to repeat all four configurations using the
project's actual airfoils, scaling, hinge paths and cut shapes without editing it.

Final Windows Debug validation passed: stabilizer_outline_tests (including GUI
failure/cancellation and box Tape/Standard geometry) and stabilizer_bevel_tests.
The separate stabilizer_bevel_tests run on the frozen BabyBuzzard .foam snapshot
also passed all four configurations at project scale. Each measured lower slope
was 1; Standard upper slopes were also 1. Tape contact error was below 0.001 mm
at the sampled stations. Logs: build/debug/bevel-final-tests.log and
build/debug/babybuzzard-all-bevels.log. Debug was rebuilt and launched; no Wing
or Fuselage generation tests ran, and the user's project was not modified.
