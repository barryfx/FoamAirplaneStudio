# ADR-0040: Shared Wingspan calibration
Status: Accepted
Date: 2026-09-21

## Context
The user requested removal of Reference's Fuselage Length field and use of
Wingspan to scale project lengths. Separate component scales could give different
physical interpretations to dimensions on the same reference drawing.

## Decision
Specify Dimensions requires only a positive finite Wingspan. The traced mirrored
wing span defines millimeters per scene unit, using the existing wing root frame.
Fuselage, stabilizers, formers, servo tray, Assembly references and Weight and
Balance use that calibration. The fuselage builder receives Side View extent
times the shared scale; existing Top/Side longitudinal registration is retained.
The physical-image option is labeled User Reference Image Scale and keeps its
metadata-based behavior.

Legacy fuselage length/text JSON fields remain readable in format 26, but are
not used for geometry or readiness. No incompatible format migration is needed.
Generation fingerprints include the derived scale/length, invalidating affected
model and mass caches when wing calibration changes.

## Alternatives Considered
Using Wingspan as a literal fuselage length would make both lengths equal.
Scaling against image bounds would count margins and unrelated views.

## Consequences
All traced views must use the same drawing scale. Older projects with differing
wing and fuselage calibrations regenerate using the wing scale. Dimensioned
stock thicknesses and explicitly entered RC part sizes remain physical values.

## Validation
Reference UI/readiness and drawing-calibration checks cover Wingspan alone,
invalid input, unit conversion, the removed field, the exact new label, and
translation/rotation invariance. Geometry tests are excluded at user request.
