# FoamAirplaneStudio

**This software has been tested through Export, but no airplanes have been milled with it.**

[Project website and downloads](https://barryfx.github.io/FoamAirplaneStudio/)

**Version 0.1.0 — October 1, 2026.** Desktop foam-airplane design with C++23,
Qt and Open CASCADE Technology (OCCT). Trace reference plans, generate separate
foam and structural parts, assemble the airplane, and export manufacturing files
for CNC routing, 3D printing and laser cutting.

Projects use `.foam` format **31**; versions 1–30 remain readable. The application
version and project-format version are independent. Legacy `.designrc` projects
are not supported.

## Platforms and Release packages

All three installers contain **Release builds**. Generated installers, source
archives and checksums are in `dist/`; they are not committed to Git.

| Platform | Installer | Validation environment |
|---|---|---|
| Windows x64 | `FoamAirplaneStudio-0.1.0-Windows-x64-Setup.exe` | Local Windows x64 installation and launch; targets Windows 10 1809 or later |
| Ubuntu x86-64 | `foamairplanestudio_0.1.0_amd64.deb` | Ubuntu 24.04.3 LTS under WSL 2 with WSLg |
| Fedora x86-64 | `foamairplanestudio-0.1.0-1.x86_64.rpm` | Fedora 44 container, normal user, Xvfb virtual X11 display |
| macOS | Not available | Planned target; no validated build, installer or run procedure |

Linux packages are built separately for each distribution. Other Linux versions
and native desktop environments are not yet validated. Linux uses X11; Wayland
sessions need XWayland, and WSL needs WSLg. See the exact completed checks in
[Release installer validation](docs/baseline/linux-package-validation.md).

## Implemented features

- **Reference:** PNG/JPG and multipage PDF plans, physical image scale or entered
  wingspan, mixed mm/in inputs, embedded reference images in saved projects.
- **Wing:** numbered panels, line/spline outlines, curve-attached stations,
  imported DAT or hand-traced airfoils, smoothing previews, and normalized DAT
  export with at most 69 cosine-sampled points. Panel dihedral, rounded tips,
  mirrored bodies, ailerons/flaps, tape/standard hinge relief, spar channels,
  split-wing alignment features and optional lightening pockets.
- **Fuselage:** Top/Side outlines, profile stations, Line/Spline/Circle profiles,
  copy/paste and movement, orphan-profile recovery, variable wall thickness,
  explicit nose/tail closure (Nose Open and Tail Closed by default), separate
  mirrored halves, alignment pins, cavity-fitted formers and servo tray.
  Formers can be rotated or partial height; their entered thickness stays fixed
  when wingspan changes.
- **Fuselage cuts and holes:** Top/Bottom/Left/Right surfaces. Boundary-crossing
  cuts split both opposite walls; closed interior cuts detach only the selected
  wall. Cut-outs remain separate components. Single-wall cuts that hit a parallel
  wall stop generation with an error. Holes remove selected-wall material.
- **Stabilizers:** independent horizontal/vertical outlines and airfoils,
  separate elevator/rudder bodies, tape/standard hinge bevels and cut shapes.
- **Assembly:** positioning, rotation about the root center in 0.5-degree steps,
  collision checks and reversible mating cuts, including wing seats in formers.
- **Inspect and export:** per-component visibility and editable export names;
  former STEP/DXF/SVG/STL and component STEP/STL export, with All selected by
  default. Combined STEP files use the project name. Tray DXF is not implemented.
- **Weight and Balance:** movable named RC parts, foam/plywood/carbon-fiber mass,
  CG and wing loading. Editable density defaults are 25.63/680/1540 kg/m³.
  Mid spars default to 6 mm OD and 5 mm ID. The CG marker is placed longitudinally
  at the calculated CG and vertically 20% up from the root airfoil's lowest point
  through its maximum thickness. CG distance is relative to the placed wing root
  leading edge, positive toward the tail.
- **Workflow:** project-wide Undo/Redo (Ctrl+Z/Ctrl+Y), individual sketch-curve
  selection/deletion, cancellable background regeneration and model caching.
  Available airplane statistics are saved with the project. Assembly and Export
  omit the statistics footer and defer mass calculations; Weight and Balance
  has its own totals. The primary toolbar ends with Inspect, Weight and Balance,
  then Export.

Detailed behavior and design decisions are indexed in the
[documentation guide](docs/README.md), [architecture](docs/architecture/baseline.md)
and [project format](docs/formats/foam-project.md). Requirements documents may
also describe future features; historical validation records describe their
recorded revisions.

## Included example

Every installer includes the `Example` folder containing **BabyBuzzard36.foam**
and its reference plan **Baby_Buzzard_Plan_559_New.pdf**.

| Installation | Example folder |
|---|---|
| Windows default | `%LOCALAPPDATA%\Programs\FoamAirplaneStudio\Example` |
| Windows custom folder | `Example` inside the chosen installation directory |
| Ubuntu and Fedora | `/usr/share/foamairplanestudio/Example` |
| Source checkout | `resources/Example` |

Use **File > Open** to open `BabyBuzzard36.foam`, then **Save As** to a folder you
own before editing. Linux installation directories are read-only for normal users.
The project opens in 2D without automatically generating geometry; select 3D or
Assembly when ready. Installed examples may be replaced during upgrades.

## Windows: install and run

Run `FoamAirplaneStudio-0.1.0-Windows-x64-Setup.exe`, review and accept the licenses,
and choose whether to create a desktop shortcut. Installation is per user and
requires no administrator rights. The default folder is
`%LOCALAPPDATA%\Programs\FoamAirplaneStudio`. Qt, OCCT and the MSVC runtime are
included; installed license files are checked against their SHA-256 hashes.

Launch FoamAirplaneStudio from the Start menu, its optional desktop shortcut, or:

```powershell
& "$env:LOCALAPPDATA\Programs\FoamAirplaneStudio\foamairplanestudio.exe"
```

Uninstall through Windows Installed Apps or `unins000.exe` in the install folder.

### Build Windows from source

Install Visual Studio 2026 C++ tools, CMake 4.2+ (required by the checked-in VS 2026
presets), Qt 6.11.1 with Qt PDF at `C:/Qt/6.11.1/msvc2022_64`, and OCCT 8.0.0.
The presets use sibling OCCT installations at
`../third_party/occt/install-debug` and `../third_party/occt/install-release`.
Adjust the preset/cache paths for your dependency installations. Inno Setup 6 is
required to create the Windows installer.

```powershell
# Debug development build and run
cmake --preset windows-debug
cmake --build --preset windows-debug --target designrc --parallel 4
.\build\debug\Debug\foamairplanestudio.exe

# Release build, run and package
cmake --preset windows-release
cmake --build --preset windows-release --target designrc --parallel 4
.\build\release\Release\foamairplanestudio.exe
pwsh -File packaging/windows/build-installer.ps1
```

Close the corresponding workspace app before rebuilding it. The internal CMake
target is still `designrc`; the executable and application identity are
`foamairplanestudio` / FoamAirplaneStudio. Packaging path overrides and silent
license acceptance are documented in [Windows installer](docs/architecture/windows-installer.md).

## Ubuntu: install and run

On Ubuntu 24.04, install the DEB with APT so dependencies are resolved:

```bash
sudo apt install ./dist/foamairplanestudio_0.1.0_amd64.deb
foamairplanestudio
```

For this Windows checkout under WSL, the package path is:
`/mnt/c/Users/barry/projects/FoamAirplaneStudio/dist/foamairplanestudio_0.1.0_amd64.deb`.
The desktop application menu also includes FoamAirplaneStudio.

## Fedora: install and run

On Fedora 44, install the RPM with DNF:

```bash
sudo dnf install ./dist/foamairplanestudio-0.1.0-1.x86_64.rpm
foamairplanestudio
```

Both Linux packages install the launcher at **`/usr/bin/foamairplanestudio`**,
which is normally already on PATH. Use this launcher: it sets the private library
and OCCT resource paths. The binary and bundled libraries are in
`/usr/lib/foamairplanestudio`, resources/examples in `/usr/share/foamairplanestudio`,
and package metadata in `/usr/share/doc/foamairplanestudio`. Licenses are beside
the binary. Linux package installation requires administrator privileges and
does not display Windows Setup's interactive license-acceptance page.

Uninstall with `sudo apt remove foamairplanestudio` (Ubuntu) or
`sudo dnf remove foamairplanestudio` (Fedora). Projects saved outside the installation
remain yours.

## Linux: build from source and create packages

Use CMake 3.24+, Ninja, a C++23 compiler, Qt 6.4+ with PDF, and **OCCT 8.0**.
Ubuntu validation uses GCC 13.3/Qt 6.4.2; Fedora uses GCC 16.1/Qt 6.11.2.
Fedora 44's system OCCT 7.9.3 is too old. Build OCCT 8 on each target distribution;
do not reuse Ubuntu binaries on Fedora.

```bash
# Ubuntu build dependencies
sudo apt update
sudo apt install build-essential cmake ninja-build git qt6-base-dev \
  qt6-base-dev-tools qt6-pdf-dev libgl-dev libglu1-mesa-dev \
  libx11-dev libxext-dev libxmu-dev libxi-dev libfreetype-dev \
  libfontconfig1-dev dpkg-dev fakeroot gzip

# Fedora build dependencies
sudo dnf install gcc-c++ cmake ninja-build git rpm-build qt6-qtbase-devel \
  qt6-qtpdf-devel mesa-libGL-devel mesa-libGLU-devel libX11-devel \
  libXext-devel libXmu-devel libXi-devel freetype-devel fontconfig-devel gzip
```

Set `occt_source` to the extracted OCCT 8.0.0 directory containing CMakeLists.txt:

```bash
occt_source="$(realpath ../third_party/occt/OCCT-8_0_0/OCCT-8_0_0)"
occt_prefix="$(realpath -m ../third_party/occt/install-linux-release)"
cmake -S "$occt_source" -B "$HOME/build/foam-occt-release" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX="$occt_prefix" \
  -DINSTALL_DIR_LAYOUT=Unix -DINSTALL_DIR_CMAKE=cmake \
  -DBUILD_MODULE_Draw=OFF -DUSE_FREETYPE=ON -DUSE_OPENGL=ON -DUSE_XLIB=ON \
  -DUSE_FFMPEG=OFF -DUSE_FREEIMAGE=OFF -DUSE_TBB=OFF -DUSE_VTK=OFF
cmake --build "$HOME/build/foam-occt-release" --parallel 4
cmake --install "$HOME/build/foam-occt-release"

# From the FoamAirplaneStudio repository, run the matching Release packager:
sh packaging/linux/build-package.sh  # Ubuntu
sh packaging/linux/build-rpm.sh      # Fedora

# Run the uninstalled Release application:
./build/linux-release/foamairplanestudio   # Ubuntu
./build/fedora-release/foamairplanestudio  # Fedora
```

Scripts use CPack and private Qt/OCCT/ICU libraries, following DesignRC's approach.
They stage on a native Linux temporary filesystem and write the package,
format-specific application-source archive, and SHA-256 checksums to `dist/`.
RPM builds accept an `OpenCASCADE_DIR` environment override; both scripts accept
`CMAKE_BUILD_PARALLEL_LEVEL` (default 4). Application-source archives do not contain
dependency source trees. See [Linux packaging](docs/architecture/linux-packages.md).

For a Debug application and selected non-generation editor tests:

```bash
cmake -S . -B build/linux-debug -G Ninja -DCMAKE_BUILD_TYPE=Debug \
  -DOpenCASCADE_DIR="$occt_prefix/cmake" -DDESIGNRC_BUILD_TESTS=ON
cmake --build build/linux-debug --target designrc reference_tests \
  sketch_editor_tests file_dialog_tests --parallel 4
LD_LIBRARY_PATH="$occt_prefix/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}" \
QT_QPA_PLATFORM=offscreen ctest --test-dir build/linux-debug \
  -R '^(reference_tests|sketch_editor_tests|file_dialog_tests)$' --output-on-failure
./build/linux-debug/foamairplanestudio
```

## Validation and licensing

Follow [AGENTS.md](AGENTS.md): Wing/Fuselage generation tests require explicit
authorization, including broader suites that generate those components internally.
Do not substitute an unfiltered CTest run for the selected editor checks above.
The legacy `designrc_geometry_tests` target remains an intentionally skipped placeholder.

See [current installer validation](docs/baseline/linux-package-validation.md),
[Release/Debug regression evidence](docs/baseline/release-debug-test-checks.md),
and [geometry validation](docs/baseline/four-surface-cut-validation.md).
Installation smoke tests do not establish full geometry or native-desktop coverage.

Copyright (C) 2026 Barry Foust. Application license: **GPL-3.0-only**; see
[LICENSE](LICENSE). Dependencies retain their own terms; see
[third-party notices](THIRD_PARTY_NOTICES.md) and [license collection](licenses/README.md).
