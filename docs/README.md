# Documentation guide

Current state: September 16, 2026.

- `requirements.md` and `GUI Design.txt` define the intended product, including
  features not yet implemented.
- `architecture/` describes current implementation. Start with `baseline.md`,
  `wing-workflow.md`, `wing-solids.md`, and `regeneration.md`.
- `formats/foam-project.md` specifies version 8 and backward migration.
- `adr/` records decisions chronologically. Later decisions supersede affected
  portions of earlier ones; introductory version numbers and validation sections
  describe each decision at the time, not necessarily current behavior.
- `baseline/` contains dated validation and investigation evidence. Earlier claims
  about synchronous generation, selectable tips, file versions, missing Git history,
  or unimplemented features describe the recorded revision. DesignRC-README.md,
  DesignRC-help.html and source-manifest.json are preserved baseline provenance.

ADR-0013 establishes independent panel modeling; ADR-0016 replaces tip choices
with dihedral and automatic rounding; ADR-0017 adds lightening; ADR-0018 replaces
synchronous regeneration with cancellable background jobs. The latest feature
validation is `baseline/regeneration-validation.md`.

Fuselage/stabilizer modeling, assembly/interfaces, export workflows and undo remain
future work. Fuselage unlocks after Wing airfoil assignment, but has no modeling
editors yet.
