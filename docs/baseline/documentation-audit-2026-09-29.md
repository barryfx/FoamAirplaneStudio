# Documentation audit — September 29, 2026

Updated current documentation and bundled help to match the pending application
changes: format 31 and legacy migrations, explicit nose/tail closure, four-surface
cuts and retained cut-outs, parallel-wall checks, partial-height formers, former
SVG export and default All selection, deferred Assembly/Export mass calculations,
and the Weight and Balance CG marker. The main README and architecture overview
now agree with the detailed implementation documents.

The Windows installer documentation records per-user installation, optional desktop
shortcut, license acceptance and installed-license hash verification. The license
README distinguishes its original Debug inventory from the fresh Release package.
Upstream license texts are unchanged. Historical ADRs and dated validation records
retain their original findings; later decisions and current architecture describe
superseding behavior.

## Validation

On September 29, the existing Debug and Release builds each passed these four
non-generation tests: `test_check_tests`, `test_check_failure_tests`,
`airplane_statistics_tests`, and `export_tests`. The failure-path test passes only
when its deliberately failing check exits unsuccessfully. License manifest hashes
and the byte-identical application GPL copy were verified. Documentation links
and Git whitespace checks were checked before commit.

No Wing/Fuselage generation tests, application rebuild, or installer rebuild were
performed during this documentation/commit task. Existing build, geometry and
installer evidence remains in:

- [Release/Debug checks](release-debug-test-checks.md)
- [Explicit fuselage ends](explicit-fuselage-ends.md)
- [Four-surface cuts](four-surface-cut-validation.md)
- [Cut clearance and partial formers](cut-clearance-partial-formers.md)
- [Windows installer](windows-installer-validation.md)

The previously built installer contains the documentation as staged at its build
time; future packaging includes this audit's updated bundled help and notices.
