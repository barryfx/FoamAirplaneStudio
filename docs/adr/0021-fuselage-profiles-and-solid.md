# ADR-0021: Station-linked fuselage profiles and independent solid generation
Status: Accepted
Date: 2026-09-16

## Context
Fuselage stations need editable cross-sections whose ownership survives moves and
deletion of other stations. The user requires independent width/height fitting,
nose registration, and generation independent from Wing.

## Decision
Add nullable stable profile slots to fuselage station records and a separate
SketchEditor collection. Persist them in backward-readable format version 11.
Use shared nose-centered coordinates and normalized longitudinal registration,
sampled corresponding sections and an OCCT ruled solid loft. Keep a separate
owned Fuselage worker, fingerprint and cached shape; retain the existing single
active-job UI lock and cancellation policy. See architecture/fuselage-profiles.md
for sampling and end treatment.

## Alternatives Considered
Array indices tied directly to station order risk reassignment on deletion.
Wing airfoil-library normalization assumes an aerodynamic chord and is unsuitable
for arbitrary body sections. A simultaneous multi-component job scheduler adds
unneeded UI/lifecycle complexity while editing is locked. Exact surface fitting
with manufacturing tolerance controls remains a future refinement.

## Consequences
Moving stations preserves ownership; unused sketch slots can remain after station
deletion. Old projects load without assignments. Cached component switching avoids
Wing regeneration. The initial solid approximates curves at documented sampling
resolution and does not yet implement shell/thickness/cut operations.

## Orientation correction (2026-09-16)
GentleLady exposed a seam ambiguity when slightly sloped profile roofs selected
opposite top corners. Correspondence now follows drawing-centered top/right/
bottom/left landmarks, with a quarter of the samples between each pair. This
preserves the user's drawn up direction without changing saved project data.
Top and Side boundaries now also guide additional longitudinal section placement,
with 0.1 mm sampled-rail interpolation tolerance. Narrow shoulders between the
original uniform sections therefore remain part of the outer shape.

## Validation
Focused Fuselage interaction/persistence and solid dimension tests, background
completion/cache checks, cancellation, Debug build and visual smoke inspection.
See baseline/fuselage-profile-validation.md for executed results.
