# Project-open responsiveness validation

2026-09-17, Windows MSVC Debug, Qt Fusion, maximized MainWindow.

Opening `reference_imgaes/GentleLady.foam` reproduced a scrollbar resize loop.
The read-only `stabilizer_outline_tests --open-only <file>` diagnostic checks
that the saved viewport is 2D before opening, so it cannot start generation.

Before the fix, Open returned in 1,588 ms, but the following approximately
3-second event-loop observation took 3,882 ms and delivered 10,223 viewport
resize events and only 9 heartbeat ticks (25 ms timer). Viewport dimensions
alternated between 1552 x 874 and 1538 x 860.

After the fix, the final build opened in 1,605 ms and the following 2,997 ms
observation delivered 2 resize events, 3 paints and 94 heartbeat ticks. No
model job ran and the project remained unmodified. The captured restored
window was visually inspected. These are single local diagnostic samples,
not a general throughput benchmark.

The synthetic saved-view regression in `sketch_editor_tests` fails with the
old bounds calculation and passes with the fix; it also checks preserved
zoom/center and that resizing stops after settling. The complete editor suite
passed in 1.29 seconds. Debug application and diagnostic targets rebuilt.
No Wing or Fuselage generation tests were run. Linux/macOS were not tested.

The input file was unchanged (SHA256):
`7A23E87A6E194946CB67AFDEF74AFD9CE586B81FF702F16B5FB692C425003376`.
