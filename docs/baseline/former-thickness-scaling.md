# Former thickness across reference scaling — 2026-09-24

Reference changes compensate each former rectangle's drawing width using the
previous and new project scales. Centers, heights and angles remain unchanged in
drawing coordinates. Geometry generation therefore receives the original physical
thickness while positions and heights follow the new model size. The rectangles
are saved normally; project/history restoration establishes their saved scale
without compensating again. Invalid intermediate Wingspan text retains the last
valid calibration. No persistent format or geometry-kernel change is required.

Validation passed using the rebuilt Debug binaries:

- `former_tests --editor-only`: GUI thickness edits, placement, rotated formers,
  reference scaling up/down, incomplete input, persistence and no regeneration.
- `former_tests --scale-project <BabyBuzzard36.foam>`: all four existing former
  thicknesses preserved at 48, 24 and 36 inches, including undo/redo and save/reopen
  through a temporary project. Centers, heights and rotations are preserved by
  the width adjustment. The original project was not changed.
- `editor_history_tests`: existing editor undo/redo checks passed.

The initial offscreen test launch could not start because this Windows deployment
contains only the Windows Qt platform plugin. All three tests were rerun with the
Windows platform and passed. Debug was rebuilt and launched successfully.

Logs are in the ignored `build/debug/former-thickness-*.log` files.

The requested full BabyBuzzard36 fuselage run passed former fitting and rail
creation/joining, then failed after 492 seconds in the hole-cutting stage:
"Move or resize the hole: its footprint must reach the inner cavity across the
whole loop to cut only one wall." No fix to that separate failure was attempted,
as requested. This run does not establish successful complete model generation.
