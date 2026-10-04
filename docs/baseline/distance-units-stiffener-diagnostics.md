# Distance units and stiffener diagnostics — 2026-10-04

## Unit retention and regression checks

Windows Debug, Qt 6.11.1, repository OCCT. Builds use `--parallel 8`.
All 11 focused tests passed: `length_display_tests`, `stiffener_tests`,
`stiffener_panel_tests`, `reference_tests`, `spar_panel_tests`, `fiberglass_tests`,
`weight_balance_tests`, `servo_tray_tests`, `former_tests`,
`fuselage_thicken_ui_tests`, and `editor_history_tests`.

The wall UI test initially asserted the superseded behavior (convert explicit mm
entries to project inches on refresh/open). Its two expectations were changed to
retain mm and its complete UI rerun passed. No production change was required by
that failure. Mixed-unit state and round-trip checks include stations, separate
formers, trays, stiffeners, fiberglass resin and balance parts. Existing reference,
spar and lightening text storage is unchanged.

The stiffener test cuts a synthetic projecting tab free, verifies an unexpected
solid-count error with the detached part spanning 30–70% from the nose, and checks
that failed cuts leave the caller's carbon-stock results unchanged. Existing strip,
round and curved-boom generation cases also pass.

## BabyBuzzard36 reproduction

Read the user's saved BabyBuzzard36.foam without editing it. Input SHA-256:
`dba4db165b5a6d76ef042af04b8163eda7489ca38137d4dfab3052c1e5150ada`.
The stored `shape: 1` means Strip (`SparShape` is Round=0, Strip=1). Settings:
one per side, width 3 mm, depth 1 mm, Start 20%, Stop 75%; mirrored construction
and OCCT parallel mode enabled. The benchmark harness now includes the saved
stiffener state when constructing its input (previously it omitted that field).

The cut completed and passed OCCT topology validation, but returned two solids
where its supported right-half input had one. The additional solid has volume
9.97167 mm³ and its X bounds span 20.42–21.14% of total fuselage length from the
nose. This is near the beginning of the 20–75% groove. Clearance/spacing checks
and tool lofts had already passed. Mirroring and later hole/cut stages were not
reached. The model remains rejected; this change improves diagnosis and does not
silently discard the extra solid or alter the cutting algorithm.

This localizes the returned fragment, not an exact break point. The evidence does
not by itself distinguish a physical sliver from a numerical Boolean artifact.
No alternative groove dimensions or ranges have been validated for this project.
The new user-visible exception reports solid counts, fragment volume and this
percentage span. Failed generation publishes no model or carbon stock.
