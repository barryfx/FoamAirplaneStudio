# GentleLady spar failure investigation

Date: 2026-09-15. Read-only diagnosis using the Windows Debug spar_tests runner.
Original: reference_imgaes/GentleLady.foam (version 6).
SHA256: B393D48F8B7A13FDCCEE41008A6C4FB2A7EBDCB457AC6F11902F0F0E012E2EB6.

## Reproduction

`build/debug/Debug/spar_tests.exe reference_imgaes/GentleLady.foam`

Fails at Panel 1 / Top spar, **100% panel span**, 527.428 mm from its root:
the 1.000 mm strip depth encounters reported thickness 0.006 mm. This particular
failure is at the outboard boundary, rather than the root. Both Top and Bottom
are 5 mm wide, 1 mm deep, at 25% chord, with 100% panel-one length.

## Findings

The root endpoints differ in image X by 0.376197 mm. The root-derived frame is
rotated 0.094082 degrees relative to the vertical panel join. At that join, LE
and TE have different projected spans by 0.374961 mm.

WingSolidBuilder samples constant projected-span sections and intersects them
against all planform edges, including temporary panel closures. Near an oblique
closure those intersections have a rapidly shrinking chord. It scales airfoil
thickness by this shortened chord and eventually adds a vertex at the maximum
span. Thus a normal panel boundary becomes an artificial narrowing wing tip.
The same construction can collapse an outer panel's oblique root and produce
`The outline closes before the wing tip`. SparCut samples almost exactly at the
maximum span for a 100%-length spar, exposing this defect.

At the full 228.352 mm panel chord, the saved AG35 coordinates imply about
19.839 mm thickness at 25% chord (linear interpolation of the saved profile).
The reported 0.006 mm is therefore not the intended full AG35 section thickness.

Separately, the first station has its leading anchor on panel-one curve 1 (the
bottom edge) and trailing anchor on curve 0 (top edge). All other stations use
top as LE and bottom as TE. This reversed first station sets an inconsistent
profile orientation; explicit station sections and automatically sampled
sections can disagree. It must be distinguished from the angled-boundary bug.

## Controlled experiments

Temporary copies are in build/debug/gentle-lady-investigation; original unchanged.
- Panel-one spars shortened to 99%: both grooves pass their panel-one checks,
  then panel two fails with `The outline closes before the wing tip`.
- Only the reversed first station corrected: panel one still fails at 100%,
  reporting 0.004 mm. The boundary defect is independent of that reversed station.
- Only root endpoints aligned in image X (and root station anchor updated):
  panel one passes both grooves; panel two reports an outside-fixed-wing spar.
- Aligning the root and correcting the station together: generation succeeds
  through both panels, all four surface grooves, mirroring and meshing. The
  runner verifies four valid solids with the original spar sizes and lengths.

## Required correction

Treat internal panel root/end boundaries as full airfoil sections with their
actual obliquity; do not shrink the airfoil as if a construction closure were
a physical tip contour. Spar endpoints must follow the actual panel boundary
at the selected chord location. Also validate inconsistent LE/TE station
orientation before lofting and give a station/panel-specific diagnostic.
No application implementation or original project data was changed in this
investigation; this records a confirmed remaining geometry limitation.

## Follow-up implementation
The subsequent station-boundary change (ADR-0015) addresses the artificial internal
end/root slivers. A temporary copy correcting only the reversed station now builds
four valid solids with the original skewed outline and spar settings and opens in
the 3D viewport. The original file remains unchanged and now receives an explicit
LE/TE conflict diagnostic until the station is redrawn. See
station-boundaries-validation.md for regression results.
