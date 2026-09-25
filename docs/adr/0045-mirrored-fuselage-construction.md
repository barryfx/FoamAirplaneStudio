# ADR-0045: Construct one fuselage half and reflect it
Status: Accepted
Date: 2026-09-22

## Context
The user requested half-fuselage construction and a GentleLady benchmark against
main. The saved traced profiles are not exactly symmetric. The user explicitly
selected one half of each profile as authoritative and clarified that the final
fuselage halves remain separate parts.

## Decision
Use the right arc of each normalized, drawn-up profile, from its top centreline
crossing through its right side to its bottom centreline crossing. Reflect those
samples to define a symmetric section. Retain the total Top View guide width and
centre it at the registered Y=0 plane; Side View height and vertical offsets remain
unchanged. Saved drawings and project format are unchanged. This intentionally
changes geometry for asymmetrically traced profiles or drifting Top centre lines.

Loft and hollow only the positive-Y half. Add its integral tray supports and former
retainers, then reflect that completed body into a separate negative-Y solid.
Do not join the main fuselage halves or split a whole fuselage afterwards.
Construct a whole inner cavity for fitting whole removable formers/tray and for
one-wall hole tooling. Joining cavity halves is construction tooling only.

Apply holes and user cuts after reflection so one-sided edits remain one-sided.
Pieces are grouped by shared seam-face area; the group with the greatest combined
volume remains the main body, including after oblique Top cuts. Only detached
groups are reunited across the centre plane, preserving whole hatch/export behavior. Apply existing pin/socket geometry
to the separate main halves. Processing, cancellation, validity checks and mesh
settings remain in effect.

The nonpersistent `FuselageSolidInput::mirrorConstruction=false` comparison path
uses the same symmetric section definition with full lofts, both sets of supports
and the existing final centre split. It provides geometry parity evidence without
conflating the user's intended symmetry change with the optimization.

Independent former fitting, per-former rail construction and the four pin-location
searches use the existing bounded worker scheduler. Each task deep-copies its CAD
operands and owns its intersection state. Pin-ray face data is loaded lazily;
oriented crossings supply rejection intervals, with exact Boolean containment
remaining the final criterion. This avoids eager whole-solid classifier caches
whose destruction delayed cancellation. Kernel calls retain the per-operation parallel setting; OCCT shares its native
thread pool safely between concurrent callers. No global pool settings change.
Results are gathered in source order.
Pin-location overlap conflicts retry in the original placement order. Pin fusion
and matching socket subtraction also run independently on their respective halves.
Wall holes/cuts complete before pin search because they define valid mating stock.
`ProcessingControl::parallel=false` retains the serial comparison path.

## Alternatives Considered
- Keep the full-body pipeline: avoids implementation changes but duplicates work.
- Optimize only already symmetric traces: preserves old geometry, but does not
  meet the user's instruction to use one profile half on GentleLady.
- Join the main halves after reflection: unnecessary work for separately exported
  halves and explicitly declined by the user.
- Mirror final exported parts: incorrectly duplicates one-sided holes and changes
  the pin/socket relationship.

## Consequences
Slight drawing asymmetry no longer produces unequal fuselage halves. Formers and
tray remain whole. Topological ordering may change; existing Inspect aliases use
solid ordinals and should be reviewed following regeneration. No persistent
format changes or new dependencies are introduced.

## Validation
See the half-fuselage benchmark report for frozen input identity, baseline timing,
full-symmetric versus half-build parity, focused tests and Debug launch results.
