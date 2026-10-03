# Stiffener clearance classifier reuse — 2026-10-03

## Method

Windows x64, Intel Core Ultra 9 185H (16 cores, 22 logical processors), MSVC
18.5, Qt 6.11.1 and repository OCCT 8.0.0 Release libraries. Only benchmark/test
executables were built and run; the application was not rebuilt or launched.

Baseline production source: commit `d207656`. The identical new
`stiffener_clearance_benchmark` harness was linked first against that code and
its executable retained, then rebuilt after the classifier reuse change.
Each binary generated a curved hollow fuselage with one stiffener per side;
the right-side groove was cut and the half mirrored. OCCT parallel processing
was enabled for both versions. No other builds/tests ran during timing.

The 200 mm fixture has a 40 mm wide, 30 mm high section, a spline-defined 5 mm
longitudinal bend, and 5 mm walls. Coverage is 20–90%. Strip stock is 3 × 1 mm;
Round is 2 mm diameter. One warm-up per shape was discarded, followed by five
measured runs. These are synthetic curved-boom results, not a measured speedup
for every project or a straight-groove shortcut.

Groove time spans the builder's cutting-stage notification to its reflection
notification: clearance checks, cutter lofts, Boolean subtraction and validation.
Total time includes full fuselage generation and display meshing. Independent
result validation and mass/centroid extraction occur outside both timed spans.

Build the harness using the windows-release preset with
`DESIGNRC_BUILD_TESTS=ON`, target `stiffener_clearance_benchmark`; run the
executable with argument `5`. Release test configuration was temporary; its
usual tests-off preset was restored afterwards.

## Results

Median wall time in milliseconds:

| Shape | Groove before | Groove after | Groove speedup | Total before | Total after | Total reduction |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Strip | 1111.820 | 226.552 | 4.91× | 1924.095 | 1165.239 | 39.44% |
| Round | 1231.976 | 216.753 | 5.68× | 3651.765 | 2709.496 | 25.80% |

Raw measurements: [before](stiffener-classifier-before.csv) and
[after](stiffener-classifier-after.csv). Groove time decreased 79.62% and 82.41%.
The total-generation gains are smaller because reflection, finishing and meshing
remain unchanged. Timings are sequential batches and subject to system noise.

## Correctness and implementation

Body volume, all centroid coordinates, solid count, carbon volume and carbon
X/Z centroids match across every before/after row at the recorded 12 significant
digits. Both shapes retain two valid solids and two material records. The
benchmark validates every result independently with BRepCheck_Analyzer.

The implementation loads each immutable body solid into a classifier once per
groove-generation call, then uses Perform for subsequent probes. For the normal
single right-half solid, 195 loads per route become one load shared by all routes.
This does not change probe
coordinates, tolerance, lofts, Boolean policy or validation. Classifiers are
locally owned and never shared by concurrent jobs or retained across edits.

Debug `stiffener_tests` exercises analytical removal volumes, multiple grooves,
mirroring, carbon properties, full straight/curved fuselage regeneration and
clearance rejection. An added opening fixture verifies that a cached classifier
rejects an outside probe after earlier inside probes, across a multi-solid body,
without publishing partial stock. The test passed in 24.40 seconds.
