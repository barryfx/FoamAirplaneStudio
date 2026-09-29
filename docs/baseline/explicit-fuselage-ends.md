# Explicit fuselage ends - Debug validation, 2026-09-25

Implemented separate Nose Open/Closed and Tail Open/Closed groups in Outline,
format-30 persistence, one-time legacy inference on GUI load, undo/redo, and
fuselage cache invalidation. Straight slanted open end edges are hollowed using
a temporary extended guide, then both body and cavity are trimmed to the drawn
plane before fitting the tray, formers and retaining rails.

## Focused checks

All five Debug CTest cases passed (78.66 seconds total):
- fuselage_outline_tests: independent radio groups, undo/redo, save/reopen,
  legacy fields, drawing interaction and lifecycle.
- fuselage_end_tests: legacy end registration and physical tolerance behavior.
- fuselage_thicken_tests and fuselage_thicken_ui_tests: existing hollowing/UI.
- fuselage_opening_tests: independent open/closed choices with an interior
  profile, material classifications at both ends, slanted nose and slanted tail.

A GUI run loaded the BabyBuzzard36 temporary copy in Outline. The captured panel
shows all four radio buttons below the existing controls and above statistics,
with Nose Open and Tail Closed selected. The Debug application was rebuilt and
launched successfully.

## Project check

Source: BabyBuzzard36.foam in the user's Baby Buzzard plan directory.
Diagnostic copy: build/debug/BabyBuzzard36-open-nose.foam. Only the end choices,
format version and initial UI mode were changed. Original SHA-256 was verified
unchanged during testing. The agent did not save or overwrite the original.
An external save at 15:04:59 changed it to version 30 with Nose Open/Tail Closed;
that later file differs from the tested snapshot only in outline editor/UI state.

The Debug run passed slanted-nose trimming, servo-tray fitting, all four formers,
and creation of all retaining rails. Former 1 no longer reports that it misses
the inner cavity. Full-run completion is recorded below.

Evidence (ignored build outputs): explicit-ends-app-build.log,
explicit-ends-tests.log, explicit-ends-gui.log, babybuzzard36-end-controls.png,
and babybuzzard36-explicit-ends.log in build/debug.

## Complete project result

The full Debug run succeeded: 8 valid solids, including all four formers, tray,
and cut fuselage pieces. Hole cuts, cut splitting, alignment-pin placement,
pins/sockets, and display meshing completed. Geometry build time was 1427.89 s;
final aggregate volume was 699693.753949835 mm^3 (includes inserts).
The command exited successfully after solid-validity checks.
