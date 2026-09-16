# Spar validation — 2026-09-14

Windows Debug, OCCT 8.0.0 and Qt 6.11.1.

SparTests passed analytical solid checks for top round, bottom strip, top strip
and bottom round grooves, chord location and root-based length, mid hole/split,
four 3 mm tabs, 0.1 mm radial/depth hole allowance and removed volume. A tapered
NACA wing with an aileron and mid spar generates six valid mirrored solids; the
hole follows local chord percentage on both sides of the root.

SparPanelTests passed independent selection, round/strip field visibility,
physical unit conversion and restoration; its captured panel was visually
inspected. ProjectTests passed version-3 round trip, version-2 defaults,
invalid mid-strip rejection, real Open/Save As, camera restoration and New/Close
reset (46.42 s). The panel regression run took 0.37 s.

Early development runs were stopped while investigating slow tab placement
classification; they are not counted as successful tests. The final implementation
checks vertical material intervals, shares alignment Boolean operations, and
reduces collinear spar centers. No baseline speedup claim is made for this new
feature. Accuracy checks use known volumes and inside/outside sample points.

Full workflow results and launch confirmation follow below.

The full workflow passed (57.29 s): all three spar positions enabled together,
top round and bottom strip grooves, mid split, Save, unchanged-model navigation
and camera retention. The captured assembled model/panel at
build/debug/spar-wing.png was visually inspected. Internal tabs are validated
by solid tests, since they are hidden in the assembled view.

Final CTest geometry run: spar_tests passed (32.25 s), control_surface_tests passed (15.84 s). All five relevant targets passed. Debug build succeeded and the rebuilt app launched, preserving existing app sessions. Linux and macOS were not tested.
