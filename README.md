# FoamAirplaneStudio

Desktop foam-airplane design using C++23, Qt and Open CASCADE Technology (OCCT).
The drawing-driven workflow creates separate foam parts for CNC routing and 3D
printing, plus outlines for laser cutting.

## Current progress

As of September 16, 2026, Reference and Wing provide:

- PNG/JPG and multipage PDF references, physical scaling, and mixed mm/in inputs.
- Numbered panel outlines, reusable line/spline editing, curve-attached stations,
  and one shared library of imported DAT and traced airfoils.
- Independent panel lofts, cumulative root dihedral, automatically rounded outer
  tips, and mirrored OCCT wing bodies.
- Panel-specific ailerons/flaps with hinge relief and end clearance; surface spar
  grooves, Mid holes, manufacturing splits and alignment features.
- Optional lightening with accessible stepped pockets, uniform crossmembers,
  retained skins and protected alignment supports.
- Portable version-8 `.foam` projects preserving embedded references, design inputs,
  drafts, mixed-unit text and view state. Versions 1-7 remain readable.
- Cancellable background regeneration with bounded panel concurrency, panel progress,
  camera retention and mirrored display-mesh reuse. Only Cancel remains interactive
  during generation; cancellation retains the previous display.

Fuselage unlocks after Wing airfoil assignment, but its modeling editors remain
unimplemented. Stabilizer modeling, assembly/interfaces, export workflows and undo
also remain future work. Assembly and Export actions are disabled. STEP and DXF/SVG
utilities remain available for reuse; STL export is future work.

The former DesignRC field-driven wing/rib pipeline and persistence were removed.
Legacy `.designrc` projects are not supported. Original DesignRC documentation is
archival, not a description of current FoamAirplaneStudio behavior.

See the [documentation guide](docs/README.md), [requirements](docs/requirements.md),
[current architecture](docs/architecture/baseline.md),
[wing generation](docs/architecture/wing-solids.md), and
[project format](docs/formats/foam-project.md).

## Build and run

Windows prerequisites: Visual Studio 2026 C++ tools, CMake 3.25 or newer,
Qt 6.11.1 with matching Qt PDF at `C:/Qt/6.11.1/msvc2022_64`, and OCCT in
`../third_party/occt/install-debug`.

```powershell
cmake --preset windows-debug
cmake --build --preset windows-debug --parallel 4
.\build\debug\Debug\foamairplanestudio.exe
```

Internal CMake targets/options and namespaces retain designrc/DESIGNRC names.
Application/settings identity is FoamAirplaneStudio. Linux presets are retained;
current FoamAirplaneStudio Linux and macOS builds are not validated.

## Validation

```powershell
ctest --preset windows-debug
```

Tests cover project lifecycle, editors, workflow, wing/control/spar/lightening
geometry, background processing, mesh orientation and retained domain/export code.
The legacy `designrc_geometry_tests` target is an explicitly skipped placeholder.

The September 16 [regeneration validation](docs/baseline/regeneration-validation.md)
records a successful Debug build, eight relevant passing suites and app launch.
Its single-sample Debug benchmarks establish geometry parity and reduced meshing
work, but do not establish an overall sequential speedup. Historical test results
validate their recorded revisions only.

Build output, SDKs and local working artwork/projects in `reference_imgaes` are
excluded from Git. Small regression fixtures remain in `tests/fixtures`.

## License

Copyright (C) 2026 Barry Foust. GNU GPL version 3 only; see [LICENSE](LICENSE).
Dependency notices are in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and
`resources/licenses`.
