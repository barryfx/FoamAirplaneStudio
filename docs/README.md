# Documentation guide

Current state: September 18, 2026.

- `requirements.md` and `GUI Design.txt` define the intended product, including
  features not yet implemented.
- `architecture/` describes current implementation. Start with `baseline.md`,
  `wing-workflow.md`, `wing-solids.md`, and `regeneration.md`.
- `formats/foam-project.md` specifies version 20 and migration from versions 1-19.
- `adr/` records decisions chronologically. Later decisions supersede affected
  portions of earlier ones; introductory version numbers and validation sections
  describe each decision at the time, not necessarily current behavior.
- `baseline/` contains dated validation and investigation evidence. Earlier claims
  about synchronous generation, selectable tips, file versions, missing Git history,
  or unimplemented features describe the recorded revision. DesignRC-README.md,
  DesignRC-help.html and source-manifest.json are preserved baseline provenance.

ADR-0013 establishes independent panel modeling; ADR-0016 replaces tip choices
with dihedral and automatic rounding; ADR-0017 adds lightening; ADR-0018 replaces
synchronous regeneration with cancellable background jobs. ADR-0019 adds fuselage
outline views. Validation is recorded in `baseline/fuselage-outline-validation.md`;
Wing regeneration evidence remains in `baseline/regeneration-validation.md`.

Stabilizer Outline editing and persistence are implemented (architecture/stabilizer-outlines.md).
Stabilizer Airfoil and cancellable solid generation are implemented (architecture/stabilizer-solids.md).
Horizontal halves join across the centerline. Hinge Line produces separate fixed
and moving bodies; closed Cut Shapes remove material and can separate more pieces.
Each stabilizer reuses its successful model cache on unchanged navigation.
See `architecture/stabilizer-hinges.md`, `architecture/stabilizer-cuts.md` and
`baseline/stabilizer-cache-validation.md`.
Assembly/interfaces, export workflows and undo remain future work. Fuselage unlocks after Wing airfoil assignment, and supports Top/Side outlines (architecture/fuselage-outlines.md, ADR-0019).
Profile Stations places vertical Side View sections (architecture/fuselage-stations.md,
ADR-0020). Edit Profiles, solid generation, Thicken, Cut, Servo Tray and Formers are implemented.

Fuselage Edit Profiles and solid generation: `architecture/fuselage-profiles.md`,
ADR-0021 and `baseline/fuselage-profile-validation.md`.

Fuselage walls: `architecture/fuselage-thickness.md`, ADR-0022 and
`baseline/fuselage-thickness-validation.md`.

Fuselage Cut: `architecture/fuselage-cuts.md`, ADR-0023 and
`baseline/fuselage-cut-validation.md`.

Servo Tray: `architecture/servo-tray.md`, ADR-0024 and
`baseline/servo-tray-validation.md`.

Formers: `architecture/formers.md`, ADR-0025 and `baseline/formers-validation.md`.

Fuselage readiness and measured topology optimization: ADR-0026 and
`baseline/fuselage-readiness-performance.md`.

Former retaining rails and left/right main-body splitting: ADR-0029,
`architecture/formers.md`, `architecture/fuselage-cuts.md` and
`baseline/former-retainers-validation.md`.

- [Stabilizer hinge lines](architecture/stabilizer-hinges.md)

- [Stabilizer Cut Shapes](architecture/stabilizer-cuts.md)

Fuselage half alignment pins: ADR-0032, `architecture/fuselage-cuts.md` and
`baseline/fuselage-alignment-validation.md`.

The latest documentation audit is recorded in
[documentation audit](baseline/documentation-audit-2026-09-18.md).
Historical test commands are evidence, not authorization to rerun generation:
follow the current restrictions in `../AGENTS.md`.
