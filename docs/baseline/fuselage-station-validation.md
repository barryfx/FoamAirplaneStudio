# Fuselage profile station validation

Date: 2026-09-16. Windows Debug, MSVC, Qt 6.11.1 / OCCT 8.

Full Debug build passed with `cmake --build --preset windows-debug --parallel 4`.
Final focused tests passed (3/3, 3.25 s):

- fuselage_outline_tests: 2.26 s. Existing outline/lifecycle checks plus one-click
  placement from both Side View edges, no Top View placement, green hover readiness,
  same-curve closed-spline pairing, strict verticality, duplicate prevention,
  endpoint movement/Escape, select/Delete, 3D edit locking, clean selection/hover,
  Save/Open restoration into Profile Stations, version-9 migration, nonvertical
  record rejection, attached-curve deletion, scale remapping, and rejection of
  ambiguous concave sections or locations beyond the outline.
- reference_tests: 0.23 s.
- designrc_gui_tests: 0.69 s.

No Wing-specific test suites were run, as requested. The Fuselage suite uses a
small existing Wing input fixture to unlock the workspace, without generating a
Wing solid. This is not a claim of Wing geometry regression coverage.

Visually inspected build/debug/fuselage-profile-stations.png: checked Profile
Stations action, readable instructions at the top of the data panel, two vertical
magenta sections meeting both Side View spline edges, visible locked Top outline,
and disabled downstream tools. Source Help and deployed Debug Help hashes match.
`git diff --check` passed. The existing VCINSTALLDIR deployment warning is nonfatal.

The workspace Debug application was stopped before building and the rebuilt app
was launched afterward. Unrelated processes were preserved. Linux/macOS were not
validated. This step implements Side View station placement; Top/Side registration,
profile-shape editing and fuselage solid generation remain future work.

Persistence uses version 10 with readers for versions 1-9. Original projects are
only upgraded on explicit Save. See ADR-0020 and architecture/fuselage-stations.md.
