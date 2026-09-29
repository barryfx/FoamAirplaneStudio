# Windows installer and Export mass deferral — 2026-09-27

Export now shares Assembly's early statistics return. It invalidates stale saved
mass values but does not measure or calculate balance when entered. The Export
GUI test clears the mass cache before switching modes and verifies that neither
the cache nor a calculation status appears.

Relevant tests, without wing/fuselage generation:

- Debug: airplane_statistics_tests 4.60 s; export_tests 10.51 s, both passed.
- Release: airplane_statistics_tests 1.38 s; export_tests 3.61 s, both passed.
- Release application and the affected test targets built successfully.

Evidence logs: build/debug/export-no-mass-tests.log,
build/release/installer-release-tests.log and installer-release-build.log.

The Inno Setup installer was built with a fresh stage, verified license hashes,
Release-only runtime DLLs and an import dependency audit. Optional D3D/DXC compiler
DLLs were excluded. The interactive license screen was inspected: refusal is the
default and Next is disabled until acceptance. Silent setup without
/ACCEPTLICENSES=yes returned 1 and installed nothing. With acceptance, installation
returned 0. Its log recorded Administrative install mode: No and successful
SHA-256 verification of every installed license file. All 170 staged files matched
their recorded hashes after installation.

The installed application launched with PATH restricted to Windows directories.
It remained responsive, displayed its Reference window, and loaded all 40 observed
bundled Qt/OCCT/FreeType/MSVC modules from its installation directory. Windows'
msvcp_win.dll is a system component, not an app-local redistributable. Uninstallation
returned 0 and removed the executable and its per-user uninstall registration.
The smoke install was isolated under build/installer and has been removed.

Smoke logs, module inventory and launch result are in
build/installer/ec6a740a2936410884a824520daf39bb/. The test session ran at Windows
medium integrity without administrative install privileges. This is not a clean
Windows VM test, nor a code-signing or public-distribution legal audit.

Artifact: dist/FoamAirplaneStudio-0.1.0-Windows-x64-Setup.exe (33.6 MiB, unsigned).
SHA-256: CF203759B9F9232CF72039265B01639EA2EC42B2FC45EEB7B01252709256418C.
The reproducible packaging entry point is packaging/windows/build-installer.ps1;
see architecture/windows-installer.md for behavior and limitations.

## Optional desktop shortcut update

The installer was rebuilt with an unchecked desktopicon task. Both silent
selection states were tested through installation and uninstallation. Opting out
created no shortcut; opting in created FoamAirplaneStudio.lnk on the current
user's redirected OneDrive Desktop, with the correct executable and working
directory. Both installs verified license hashes and both uninstalls succeeded;
the shortcut was removed. Existing installations/shortcuts were checked before
testing and none were present. Evidence is in the latest stage identified by
dist/installer-build.json and build/release/installer-desktop-package.log.

Updated installer SHA-256:
E7AD6BE03F579D4A5D9C290C46316DB40A9C5910199F11767CECFA73481C29BD.
