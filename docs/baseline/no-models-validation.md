# Input-only project persistence validation

Date: 2026-09-19. Windows Debug, branch Assembly.

- Debug application and persistence/UI test executable rebuilt successfully.
- Ran only `ctest --preset windows-debug -R '^project_persistence_ui_tests$' --timeout 120`:
  passed in 1.55 seconds. No geometry tests ran.
- Empty compound handles simulate populated component/accessory/Assembly session
  caches. Save emits version 23 with no `models` field and retains session caches.
- Version-22 fixtures with corrupt cache objects, invalid cache strings and null
  caches open successfully. No BREP/base64/decompression validation is attempted.
  Reopened caches are empty, 2D is selected, and no generation jobs start.
- Assembly placements/cut intent survive, and resaving removes old model payloads.
  Version 21 still reads; invalid design inputs remain rejected.
- Fixed Assembly-to-Fuselage 2D restoration so active sketch layer matches the
  selected view and a subsequent Save validates. Pending drafts keep their layer.
- `git diff --check` passed. No save-time speedup is quantified.

Opening an old large JSON file still reads/parses its payload. Resaving removes
that payload. Model generation remains deferred until explicit 3D/Assembly entry.
