# Documentation audit: September 19, 2026

Audited current documentation against the Assembly, persistence, viewport,
parallel generation and former-rotation implementation before merging Assembly.

Verified current behavior:

- Assembly prepares missing components concurrently, provides the reference
  underlay and mouse controls, and preserves original/cut snapshots for Undo.
  Only rudder/elevator overlap blocks cuts. Export UI remains future work.
- Open selects 2D without generation; a saved Assembly workspace falls back to
  Fuselage/Outline/Side View. Explicit 3D/Assembly entry prepares models.
- Format 24 stores inputs, Assembly placements/cut intent and per-former angles.
  Versions 1-23 remain readable; version-22 embedded models are ignored.
- Former angles rotate masks, fitted solids and retaining slabs about the mask
  center, with negative degrees counter-clockwise in Side View.
- Wing and Fuselage parallel-generation descriptions agree with the source and
  dated benchmark evidence; single-run measurements are not universal promises.

Corrected the README's readable-version range, clarified project Open versus
Assembly-tab entry, added rotation to the GUI design and requirements, and
linked the latest complete validation results. Repaired stray Windows punctuation
bytes in the format specification and Assembly validation report. Current docs
and README pass UTF-8 and relative Markdown-link checks. The two original relative
license links in the preserved `baseline/DesignRC-README.md` remain archival
references; the document is intentionally retained as baseline provenance.

Historical ADRs and dated validation reports retain their original version and
test claims. ADR-0034 is explicitly superseded by ADR-0035; ADR-0036 adds format 24.

Validation uses the immediately preceding successful Debug builds, component
benchmarks/parity checks, and full CTest run plus corrected station-workflow rerun:
35 implemented tests passed; one intentionally empty placeholder was skipped.
See [former rotation validation](former-rotation-validation.md),
[Fuselage parallel validation](fuselage-parallel-validation.md), and
[Wing parallel validation](wing-kernel-parallel-validation.md).
This audit changes documentation only; generation tests were not rerun.
