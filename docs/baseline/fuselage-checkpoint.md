# Fuselage support checkpoint validation — 2026-10-04

## Method

Windows x64, Intel Core Ultra 9 185H (16 cores, 22 logical processors), MSVC
18.5, Qt 6.11.1, repository OCCT, Debug configuration. OCCT parallel operations
were enabled. No other build, test or application ran during these measurements.
The successfully corrected validation run preceded three measured repetitions:

```
ctest --preset windows-debug -R '^fuselage_cache_tests$' --repeat until-fail:3 -V
```

The synthetic fixture is a 200 mm rectangular hollow fuselage, 40 mm wide and
30 mm high, with 5 mm walls, one former with retaining rails, and a servo tray.
Each repetition first generates the base checkpoint, then compares independent
stiffener, side-hole and closed side-hatch edits against fresh builds of the same
inputs. Each measured interval includes all work in `buildFuselageModel`, including
copying cached geometry, downstream finishing and display meshing. Input setup
and independent parity checks are outside the interval. Cached runs precede fresh
runs. Raw samples are in `fuselage-checkpoint.csv`.

These are short synthetic Debug measurements, not Release performance guarantees
or measured improvements for every project. Fresh builds also include the new
checkpoint preparation and copying; this is not a before/after cold-build benchmark.

## Results

| Edit | Fresh median (ms) | Cached median (ms) | Reduction |
| --- | ---: | ---: | ---: |
| Stiffener | 3901.60 | 736.29 | 81.1% |
| Side hole | 3881.40 | 726.35 | 81.3% |
| Side hatch cut | 3989.33 | 778.80 | 80.5% |

All measured runs passed topology, volume, centroid, bounds, solid-count, removable
insert and stiffener-stock parity checks. Reuse also passed upstream invalidation,
failed/cancelled-work isolation, repeated reuse and first-hole-without-inserts checks.

## Full regression validation

The complete Debug CTest suite ran all 63 entries, including Wing and Fuselage
generation, in 1488.16 seconds. All 61 existing active entries passed; the empty
`designrc_geometry_tests` placeholder returned its configured skip code 77.
The new cache test initially contained an open cut whose endpoint stayed inside
the outline and therefore correctly failed to separate a body. It was corrected
to a closed side-wall hatch, rebuilt with `--parallel 8`, and passed its full
25.18-second rerun plus all three repetitions above (77.04 seconds total).
Final coverage: all 62 active tests passed; one intentional placeholder skipped.
