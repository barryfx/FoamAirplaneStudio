# Cache persistence and Assembly mouse validation

Date: 2026-09-19. Windows Debug, branch Assembly.

Historical results for format 22. Persistent caches were subsequently retired;
see no-models-validation.md for the current behavior.

- Built `designrc` and the `assembly_tests` executable successfully.
- Ran only `ctest --preset windows-debug -R '^cache_ui_tests$' --timeout 120`.
  The dedicated `--cache-ui` path passed in 2.65 seconds. It does not execute the
  Assembly geometry test path, model builders, cutters or meshing algorithms.
- Fixture: one rectangular face with hand-authored triangle data used as cached
  model records. Save/open preserves topology/triangulation and all five groups:
  Wing, Fuselage (body/tray/top faces/formers), horizontal, vertical, and original/
  cut Assembly. All four component 3D views and Assembly reuse restored caches
  without starting model jobs. Open remains in 2D and the document remains clean.
- Invalid source fingerprints do not restore stale caches. Corrupt checksums are
  rejected; version-21 files still decode with empty model caches.
- Simulated wheel-up reduces camera scale while preserving the point under the
  cursor; right-drag changes camera center and left-drag changes camera direction.
  Inspected `build/debug/cache-ui.png` showing the restored mesh and reference
  after mouse navigation. Physical-pixel conversion supports scaled displays;
  only the current display configuration was exercised.
- `git diff --check` passed. No geometry tests were run for this prompt, as
  requested; the narrowed Rudder/Elevator-only collision gate was reviewed in
  source and compiled, not exercised with Boolean collision fixtures.

Format 22 preserves BREP version-3 topology plus triangulations/normals. Old files
cannot supply caches they never stored; save after generating models to embed
those caches. Real-aircraft cache size and save/load performance were not measured.
