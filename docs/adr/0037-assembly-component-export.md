# ADR-0037: Export named Assembly manufacturing parts
Status: Accepted
Date: 2026-09-19

## Context
Export must use generated Assembly geometry, including interface cuts, and offer
separate former STEP/DXF/STL and component STEP/STL choices. Assembly previously combined
fuselage accessories into a compound, losing their identity after Boolean cuts.

## Decision
Carry body records separately from standalone former/tray inserts through Assembly
placement and cuts. Never fuse inserts to the body or apply seat cuts to them.
Retain former planes alongside their unchanged solids. Generate former numbers from nose-to-tail mask centers. Export the current
Assembly snapshot only, using the inherited DesignRC AP242 STEPCAF writer with an
aircraft hierarchy option, OCCT binary STL, and the existing DXF writer. DXF uses
the rotated former mid-plane section with 0.02 mm polyline deflection. No generated
geometry or export preferences are added to project persistence. Stage conversion
before atomic per-file replacement and retain the shared folder-dialog history.
Default both format groups to STEP. Selected formers and other STEP components
share one Components.step, retaining separate named solids within its assembly.

## Alternatives Considered
Exporting component caches would lose Assembly cuts/placement. Matching finished
solids by index or bounding box cannot reliably identify a split former. Projected
silhouettes can alter tilted-former dimensions; local sections retain physical scale.
A second STEP writer would duplicate the requested DesignRC assembly format code.

## Consequences
Export unlocks after successful Assembly preparation, with or without optional
interface cuts. Former outlines describe the center profile; machining bevel/kerf
allowances remain outside this workflow. Components use one STEP file or individual
STLs. Former DXFs/STLs are individual files. Existing project inputs remain compatible.
No new external dependency or change to component-generation algorithms is required.

## Validation
Focused synthetic Assembly/export checks cover role preservation, cut geometry,
former ordering/rotation/holes, STEP round-trip solids/volume, STL triangle volume,
UI selection, folder history and project lifecycle. See baseline/export-validation.md.
