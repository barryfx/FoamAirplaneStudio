# Airplane statistics validation

Date: 2026-09-25

Built Debug `designrc` and `airplane_statistics_tests`. The dedicated suite passed
without any wing, fuselage, stabilizer, or Assembly geometry generation.

Checked nominal wing area/span/root chord/aspect ratio, mirrored horizontal and
single vertical stabilizer areas, fuselage Side View length, quadratic area
scaling, to-scale Reference, incomplete outlines, two-panel closure, and a
single-panel outline before stations are added. Synthetic material measurements
verify total weight, CG, metric/imperial Wing Loading, saved/reopened summaries,
density updates without solids, geometry-change invalidation, legacy files
without statistics, invalid serialized values, and reset behavior.

GUI checks confirm every applicable panel owns a footer, Wing Outline tabs end
above the footer, and Airfoils/Assembly/Export bottom actions follow it. Weight
and Balance omits the footer and includes its own Wing Loading line. Visually
reviewed `build/debug/airplane-statistics-airfoils.png` and
`build/debug/airplane-statistics-balance.png`.

Logs: `build/debug/airplane-statistics-final-build.log` and
`build/debug/airplane-statistics-tests.log`. Whitespace validation passed.
Debug application rebuilt and launched after checks.
