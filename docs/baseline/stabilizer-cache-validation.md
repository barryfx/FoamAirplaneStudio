# Stabilizer cache and tape-face checks

Windows MSVC Debug, 2026-09-18. Rebuilt designrc and stabilizer_outline_tests.
The stabilizer-only suite passed in 171.98 s. Worker-start counters remain unchanged
on repeated 2D/3D navigation and returning from another workspace, independently
for horizontal and vertical components with Cut Shapes present. Cache entries now
record only successfully published geometry, preserving the prior successful
entry across cancellation or failure.

A known-thickness Tape test verifies material next to the hinge remains at +Z
(Top) and is absent at -Z (Bottom). The current Tape direction was not changed:
GentleLady.foam was observed with horizontal Standard Hinge selected, and the
reported orientation discrepancy needs confirmation against Tape selection.
The user's project file was read only. No Wing/Fuselage generation tests ran.
Debug was rebuilt and relaunched. Linux/macOS were not tested.
