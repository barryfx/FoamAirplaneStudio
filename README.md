# FoamAirplaneStudio

Desktop foam-airplane design using C++23, Qt and Open CASCADE Technology (OCCT).
The drawing-driven workflow creates separate foam parts for CNC routing and 3D
printing, plus outlines for laser cutting.

## Current progress

As of September 20, 2026, Reference and Wing provide:

- PNG/JPG and multipage PDF references, physical scaling, and mixed mm/in inputs.
- Numbered panel outlines, reusable line/spline editing, curve-attached stations,
  and one shared library of imported DAT and traced airfoils.
- Independent panel lofts, cumulative root dihedral, automatically rounded outer
  tips, and mirrored OCCT wing bodies.
- Panel-specific ailerons/flaps with hinge relief and end clearance; surface spar
  grooves, Mid holes, manufacturing splits and alignment features.
- Optional lightening with accessible stepped pockets, uniform crossmembers,
  retained skins and protected alignment supports.
- Portable version-25 `.foam` projects preserving embedded references, design inputs,
  drafts, mixed-unit text, view state. Versions 1-24 remain readable.
- Cancellable background regeneration with bounded panel concurrency, panel progress,
  camera retention and mirrored display-mesh reuse. Only Cancel remains interactive
  during generation; cancellation retains the previous display.

Fuselage unlocks after Wing airfoil assignment and provides Top/Side closed-outline
editing with the reusable sketch editor. Two valid outlines enable Profile Stations;
it places vertical Side View sections with one click. Edit Profiles attaches section sketches
to stations and generates a cached solid in a separate worker. Generation uses default/saved station walls without visiting optional tabs. Thicken edits their smooth transitions. Complete fuselage definitions enable Horiz Stab and Vert Stab navigation. Cut splits solid or hollow bodies along saved Top/Side Line and Spline paths. Servo Tray adds a cavity-fitted tray and 5 mm side ledges.
Formers adds movable, individually rotated full/partial-height cavity-fitted inserts with overlap prevention and 4 x 3 mm retaining rails on both inner sides. The largest post-cut body splits into left/right halves with four alignment pins (two top, two bottom); other cut-outs remain whole. Pins project 3 mm into 3.5 mm-deep sockets and use a 4 mm diameter capped by local wall thickness. Rail generation skips disjoint insert cuts and joins all rails in one shell fusion; measured GentleLady results are in `docs/baseline/former-retainers-validation.md`.

Holes adds closed Line/Spline loops for Top, Bottom, Left or Right, cutting only
the selected wall. Holes and Cut share controls for adding, selecting and deleting
multiple paths. Rear alignment pins start at 75% of the main-body length and
front pins at 15%, with nearby safe positions used when needed.

Horiz Stab and Vert Stab provide independent open outlines, explicit leading-edge
selection, a bundled NACA009 or imported DAT airfoil, and cached background
generation. Horizontal halves join into one stabilizer; the vertical fin is one
body. Hinge Line separates the elevator/rudder with Tape or Standard relief,
leaving two bodies. Closed Line/Spline Cut Shapes remove material through the
thickness and can create additional bodies. Unchanged navigation reuses each
component's successful model cache. Assembly provides side-view positioning,
control-surface collision checks, reversible intersection cuts and separate export
geometry caches (see [Assembly](docs/architecture/assembly.md)). Export offers former STEP/DXF/STL and component STEP/STL from the current Assembly
(see [Export](docs/architecture/export.md)). General editor undo, SVG and tray DXF
remain future work; Assembly includes Undo Cuts.

The former DesignRC field-driven wing/rib pipeline and persistence were removed.
Legacy `.designrc` projects are not supported. Original DesignRC documentation is
archival, not a description of current FoamAirplaneStudio behavior.

See the [documentation guide](docs/README.md), [requirements](docs/requirements.md),
[current architecture](docs/architecture/baseline.md),
[wing generation](docs/architecture/wing-solids.md), and
[project format](docs/formats/foam-project.md).

## Build and run

Windows prerequisites: Visual Studio 2026 C++ tools, CMake 4.2 or newer
(required by the preset's Visual Studio 2026 generator),
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
ctest --preset windows-debug -R "^(reference_tests|sketch_editor_tests|wing_workflow_tests)$"
```

This is a small non-generation editor/reference check. Follow `AGENTS.md`:
Wing and Fuselage generation tests require explicit user authorization, including
suites that generate those components internally. Unfiltered CTest includes them.

The available tests cover project lifecycle, editors, workflow, wing/control/spar/lightening
geometry, background processing, mesh orientation and retained domain/export code.
The legacy `designrc_geometry_tests` target is an explicitly skipped placeholder.

The September 19 [former rotation validation](docs/baseline/former-rotation-validation.md)
records the full Debug build, 35 passing implemented tests after the workflow-test
update, one intentionally skipped placeholder, and app launch. Parallel generation
measurements are recorded for [Fuselage](docs/baseline/fuselage-parallel-validation.md)
and [Wing](docs/baseline/wing-kernel-parallel-validation.md).

The September 16 [regeneration validation](docs/baseline/regeneration-validation.md)
records a successful Debug build, eight relevant passing suites and app launch.
Its single-sample Debug benchmarks establish geometry parity and reduced meshing
work, but do not establish an overall sequential speedup. Historical test results
validate their recorded revisions only. Fuselage validation is recorded in
[fuselage outline validation](docs/baseline/fuselage-outline-validation.md) and
[profile station validation](docs/baseline/fuselage-station-validation.md).

Build output, SDKs and local working artwork/projects in `reference_imgaes` are
excluded from Git. Small regression fixtures remain in `tests/fixtures`.

## License

Copyright (C) 2026 Barry Foust. GNU GPL version 3 only; see [LICENSE](LICENSE).
Dependency notices are in [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md) and
`resources/licenses`.
