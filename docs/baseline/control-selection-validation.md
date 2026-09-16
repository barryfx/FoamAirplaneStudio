# Control rectangle selection validation
Date: 2026-09-14

ControlSurfaceEditorTests exercises selection of both rectangles, switching
selection, Escape, Delete with and without a selection, retaining checkbox and
hinge settings after deletion, seven-pixel edge tolerance at increased zoom,
empty-space deselection, mode locking and drawing isolation. Selection and
Escape without a pending corner produce no data-change callback; deletion does.
The panel instructions are checked for the actual selection controls.

The Debug editor test passed. The capture at
build/debug/control-selection.png was visually inspected: selected aileron has
a thicker cyan border and stronger translucent fill, while the unselected flap
retains orange styling. Full Debug build succeeded. Project, workflow and other
sketch editor regression results are recorded below.

Project, station-sketch and outline-sketch tests passed. The first workflow run
failed because it asserted the previous status-bar wording; its assertion was
updated to check the new Delete/Escape instructions before rerunning.

Workflow rerun passed (21.46 seconds). All five relevant test targets passed. Rebuilt Debug app launched successfully; existing app sessions were preserved.

Empty-control cleanup: editor tests passed (0.38 s), project tests passed (8.84 s), and full workflow passed (28.12 s). Tests cover deleted aileron cleanup on 3D entry, deleted flap cleanup on mode exit, retaining defined controls, and discarding incomplete drafts. Debug rebuilt and launched successfully with existing sessions preserved.
