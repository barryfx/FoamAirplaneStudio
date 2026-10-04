# Documentation guide

The [project website](architecture/project-website.md) is checked into `website/`
and deployed from `main` through GitHub Pages.

Current state: October 4, 2026 (application 0.2.0, project format 33).

All three 0.2.0 installers have been [built and tested locally](baseline/installer-0.2.0-validation.md).
Published GitHub installer assets remain at 0.1.0.

- `requirements.md` and `GUI Design.txt` define the intended product, including
  features not yet implemented.
- `architecture/` describes current implementation. Start with `baseline.md`,
  `wing-workflow.md`, `wing-solids.md`, and `regeneration.md`.
  `architecture/fuselage-stiffeners.md` describes mirrored carbon grooves.
  `architecture/fiberglass.md` describes covering shapes and their weight/CG estimates.
- `formats/foam-project.md` specifies version 33 and migration from versions 1-32.
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
Assembly positioning, collision checks and reversible interface cuts are implemented
([Assembly](architecture/assembly.md), ADR-0033). [Export](architecture/export.md) provides former STEP/DXF/SVG/STL and component STEP/STL.
Project-wide editor Undo/Redo is implemented (architecture/editor-history.md). Fuselage unlocks after Wing airfoil assignment, and supports Top/Side outlines (architecture/fuselage-outlines.md, ADR-0019).
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
[documentation audit](baseline/documentation-audit-2026-10-04.md).
Historical test commands are evidence, not authorization to rerun generation:
follow the current restrictions in `../AGENTS.md`.

Assembly implementation and reference underlay: [validation](baseline/assembly-validation.md)
and [ADR-0033](adr/0033-assembly-placement-and-interface-caches.md).

[Retiring persistent caches](adr/0035-retire-persistent-model-caches.md) describes format 23.

[Cache and mouse validation](baseline/cache-ui-validation.md).

[Input-only persistence validation](baseline/no-models-validation.md).

[Shared Fuselage parallel generation measurements](baseline/fuselage-parallel-validation.md).

[Wing internal parallelism measurements](baseline/wing-kernel-parallel-validation.md).

Former rotation: [design](adr/0036-former-rotation.md) and
[validation](baseline/former-rotation-validation.md).

[Fuselage Holes](architecture/fuselage-holes.md) removes closed loops through a
selected wall and shares path controls with Cut (ADR-0038, project version 25).

[Weight and Balance](architecture/weight-and-balance.md) places RC parts in Side View and combines their masses with foam, Aero Plywood, carbon-fiber spars/stiffeners and fiberglass cloth/resin from current Assembly geometry, including wing loading (ADR-0039 and ADR-0046; introduced in format 26, extended in format 29).

- `architecture/inspect.md` describes component visibility and persistent export names.

Project-wide Undo/Redo and individual curve selection/deletion are described in
`architecture/editor-history.md` and ADR-0043.

Circle/profile copy and movement, orphan recovery/deletion, and tolerant fuselage
end registration are described in `architecture/fuselage-profiles.md`,
`architecture/fuselage-thickness.md`, ADR-0042 and ADR-0044.

[ADR-0045](adr/0045-mirrored-fuselage-construction.md) describes right-half fuselage
construction and reflection, with whole inserts and separate main halves.

Half-fuselage timing and geometry parity: [validation](baseline/fuselage-speedup-validation.md).

- [BabyBuzzard thickening and retry regression](baseline/babybuzzard-thickening-regression.md)

- [Fuselage cancellation validation](baseline/fuselage-cancellation-validation.md)

- `adr/0046-carbon-fiber-spar-mass.md` describes automatic spar material accounting and tube dimensions.

- `architecture/airplane-statistics.md` and `adr/0047-persistent-airplane-statistics.md` describe shared summaries, Wing Loading and persistent measurement validity.

Dependency license texts, component coverage and compatibility notes are in
[the license collection](../licenses/README.md).

[Regression test conventions](../tests/README.md) describe checks that stay active
in Release builds and the command for enabling the Release test suite.

Explicit fuselage end controls and slanted open ends are documented in [ADR-0048](adr/0048-explicit-fuselage-ends.md).

Four-surface cuts, retained cut-outs and parallel-wall checks: [ADR-0049](adr/0049-four-surface-fuselage-cuts.md), [validation](baseline/four-surface-cut-validation.md), and [partial former validation](baseline/cut-clearance-partial-formers.md).

Windows per-user packaging, license verification and optional desktop shortcut: [installer](architecture/windows-installer.md) and [validation](baseline/windows-installer-validation.md).

Release and Debug regression evidence: [test checks](baseline/release-debug-test-checks.md). Only Weight and Balance recalculates mass statistics; other modes show dashes for weight, loading and CG. Assembly and Export omit the statistics footer; Weight and Balance displays its own totals and a small CG marker.

Linux DEB/RPM packaging: [design](architecture/linux-packages.md) and [installation validation](baseline/linux-package-validation.md).

Source release [0.2.0](releases/0.2.0.md) includes incremental mass caches
([ADR-0051](adr/0051-incremental-weight-balance-measurements.md)), the support-rail
checkpoint ([ADR-0053](adr/0053-fuselage-support-checkpoint.md)), entered distance
units ([ADR-0054](adr/0054-entered-distance-units.md)), and straight-side stiffener
routes ([ADR-0056](adr/0056-straight-side-stiffeners.md)).
