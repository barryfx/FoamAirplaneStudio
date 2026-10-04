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

## Follow-up: cause and proposed correction

The instrumented Debug reproduction on the same unmodified project captured the
right-half body, original smooth cutter and cut result under the ignored
`build/debug/stiffener-diagnosis/` directory. The original cut reproduced exactly
the 9.97166573975 mm³ extra solid. Temporary instrumentation and candidate code
were removed after the investigation; this is a proposal, not a production fix.

The smooth cubic cutter departs dramatically from the sampled route. At 20.5%
from the nose (X = 134.079806328 mm), the intended route midpoint is approximately
Z = -9.1861 mm, so the 3 mm strip should span roughly -10.6861 to -7.6861 mm.
A vertical ray at Y = 24 mm instead intersects the original cutter at
Z = 47.1312174597 and 50.131217425 mm. At Y = 26 mm that same X has three separate
cutter intervals, demonstrating that the smooth loft doubles back. Ordinary
`BRepCheck_Analyzer` validity passes for this cutter; the current 65 point checks
validate the input samples, not the resulting interpolated solid. All 65 samples
survived the collinearity filter, ruling out sample removal as the cause here.

The Boolean result also retains material inside the cutter. At
X = 135.905267701 mm, Z = -19.4545243331 mm, lateral ray intersections are:

| Shape | Y interval, mm |
| --- | --- |
| Original body | 15.021568834 to 24.7543165414 |
| Cutter | 23.847844202 to 27.8478442099 |
| Main cut solid | 15.021568834 to 23.847844202 |
| Extra solid | 23.847844202 to 24.7543165414 |

Thus the extra solid at this cross-section occupies the very portion that should
have been removed. This is not evidence that a correctly routed 1 mm groove
penetrated the wall. The malformed route and incorrect Boolean result are the
observed failure; the exact internal OCCT numerical mechanism was not isolated.
A larger point-classification grid was stopped after the conclusive ray checks;
no complete grid-validation result is claimed.

The nominal wall sections at 20.0479%, 20.9744% and 21.875% are respectively
6.762, 6.72926 and 6.64593 mm in this saved file. The fragment lies in that
transition, rather than at an 8 mm station. This still provides ample nominal
wall depth for a correctly routed 1 mm groove.

A diagnostic-only experiment changed the stiffener loft from smooth to ruled
(straight interpolation between consecutive sections), retaining the same
65 samples, strip settings, pre-cut body and parallel Boolean mode. It produced
one valid solid, volume 345148.040712 mm³, from a valid one-solid input of
346217.745658 mm³. It passed the existing clearance, topology and solid-count
checks. This validates the correction for the isolated failing cut, not every
subsequent fuselage stage or every other model.

Proposed production fix: replace the unconstrained smooth loft with a route
construction that is monotone along X and cannot overshoot between its sections.
Ruled interpolation is the demonstrated baseline; adaptive section spacing can
limit faceting on curved booms. If a smooth surface is retained, constrain its
route and validate the finished geometry rather than only the input samples.
Apply the same route to groove tools and carbon stock. Add regression checks for
this route, strip/round cases, groove location and depth between samples, and
absence of retained material inside the cutter. Keep the disconnected-solid
check; do not silently delete the extra solid or advise smaller stock as a fix.
