# Fuselage end registration validation — 2026-09-22

Debug application and fuselage_end_tests built successfully.
Focused CTest passed (0.13 seconds test time). Checks include both ends,
physical scale, 0.5 mm/2 degree limits, strict behavior for intentional slopes,
curved/pointed ends, inward station positions, alignment and input preservation.

The same test read BabyBuzzard.foam without saving it. Both nose edges now qualify;
the foremost profile is recognized as an open nose. Registered end extents are
50.9693 drawing mm in Side View and 41.9767 drawing mm in Top View (before the
Top View's existing length normalization). Original project inputs were unchanged.

This validates the preprocessing and open-end classification used by the builder,
not a regenerated final solid. No Wing/Fuselage solid-generation tests were run,
per the standing repository restriction. Debug was rebuilt and relaunched.

Logs: build/debug/fuselage-end-build.log and fuselage-end-test-build.log.
