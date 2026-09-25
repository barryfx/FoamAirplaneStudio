# BabyBuzzard36 former 4 right-side rails — 2026-09-23

Reproduced from a read-only snapshot of the user's BabyBuzzard36.foam at 36-inch
wingspan. Main wall hollowing and all four removable formers completed. The
front-right rail of former 4 failed the retained-material check.

The captured pocket and padded clearance cutter are both valid. At fuzzy
tolerance 1e-7 mm, OCCT reports a successful, valid Cut but returns the entire
3126.802 mm3 pocket, with its interior center inside the cutter. Using the full
translated cavity instead of the short padded cutter also retains the pocket.
At 1e-5 mm, either cutter produces the intended 417.462 mm3 rail with a clear
center. This demonstrates a numerical Boolean failure, not invalid former input.

The first full rerun cleared the front rail but exposed the same problem on the
rear rail. Its 2943.86 mm3 pocket remained uncut at 1e-7, 1e-6 and 1e-5 mm; at
1e-4 mm it produced a valid 401.579 mm3 rail without a retained plug. Testing
1e-3 mm changed that volume, so production does not permit that larger tolerance.

Production retains the original tolerance as the first attempt. A failed cut,
invalid result or retained plug triggers bounded retries at 1e-5 then 1e-4 mm. Acceptance still
requires clear interior centers and valid topology; errors stop generation.
The rail dimensions and source shapes do not change. All attempts propagate
cancellation. Temporary capture instrumentation has been removed.

The message now contains only the former number (nose-to-tail), front/rear rail,
side, a short explanation and a report/retry instruction. It omits measurements.

## Validation

The initial three focused suites passed in 96.25 seconds: `former_diagnostics_tests`,
`former_geometry_tests`, and `former_rail_cut_tests`. The captured regression
checks 1,995 point locations per run against independent pocket/cutter membership
in serial and parallel modes: 225 retained samples and 1,618 cleared samples in
each. Both return 417.462 mm3, valid topology and unchanged source volume.
Cancellation, straight/rotated/tapered rails and insert clearances are covered.
Fixtures retain only the small failing front and rear operands, not the complete model.

Final focused validation passed all four suites in 155.44 seconds: diagnostic
message checks, existing former geometry, and the captured front and rear rail
regressions. Each captured cut is checked in serial and parallel modes for valid
topology, expected volume, independent point membership and unchanged inputs.

The final complete-project attempt passed rail creation, joined all rails to the
walls and reflected the completed right half at 469 seconds. It then stopped in
the separate fuselage-hole stage: "Move or resize the hole: its footprint must
reach the inner cavity across the whole loop to cut only one wall." Therefore
this run confirms the rail failure is resolved in the actual project, but does
not establish successful end-to-end fuselage or Assembly generation. The hole
check needs separate investigation; this evidence alone does not distinguish
hole placement from another numerical Boolean issue. The original project was
not modified. The Debug application was rebuilt successfully.

Evidence in the ignored build directory: `rail-repro.log`,
`rail-cut-investigation.log`, `rail-fix-tests.log`, `rail-fixed-project.log`, and
`rail-fix-debug-build.log`. Final runs use `rail-final-build.log`,
`rail-final-tests.log` and `rail-final-project.log`.
