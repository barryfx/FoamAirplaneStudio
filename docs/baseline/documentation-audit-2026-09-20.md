# Documentation audit — 2026-09-20

Reviewed the repository documentation against the Export branch changes:
Assembly component export, standalone inserts, former rail clearance, four-wall
Holes, multiple Cut paths, project format 25 and rear alignment-pin placement.

Corrected current-format references in the README, documentation guide,
architecture pages, format specification and bundled help. Added the missing
fuselageHoles schema row, documented export directory history, and corrected
help to include Former STEP and both STEP defaults. The former rail investigation
now links the subsequently authorized regression results. Historical validation
reports and DesignRC provenance retain the behavior and results of their dates.

Current implementation references:

- [Export](../architecture/export.md) and ADR-0037: current Assembly snapshot,
  independent inserts, former STEP/DXF/STL and component STEP/STL.
- [Holes](../architecture/fuselage-holes.md) and ADR-0038: four walls, closed
  loops, one-wall cutting, shared path controls and format-25 persistence.
- [Alignment](../architecture/fuselage-cuts.md): 15% front and 75% rear targets.

Validation evidence is recorded in export-validation.md,
fuselage-holes-validation.md and fuselage-alignment-validation.md. Their focused
tests passed before this documentation-only audit; no broader generation tests
were rerun. The Debug application was rebuilt to include the updated help.
