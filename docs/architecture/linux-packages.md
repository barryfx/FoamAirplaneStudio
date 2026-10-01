# Linux DEB and RPM packages

Packaging follows the sibling DesignRC layout, isolated in
`packaging/linux/Packaging.cmake`. Linux-only inclusion leaves Windows packaging
unchanged. The internal application target remains `designrc`; installed commands,
paths, metadata and desktop integration use FoamAirplaneStudio.

CPack produces native x86-64 DEB/RPM packages. The launcher sets OCCT resource paths
and a private library path. The binary uses inherited ELF RPATH so transitive OCCT
libraries resolve beside it. Qt Core/Gui/Widgets/DBus/Network/Pdf/XcbQpa, XCB and
offscreen platform plugins plus the JPEG image plugin, OCCT libraries/resources, and ICU are bundled; other system
libraries are resolved by automatically generated APT/DNF dependencies. The
application uses X11/XWayland. Packages are built separately on each distribution.

OCCT 8 is required. Fedora's 7.9 development package cannot compile the application's
OCCT 8 container APIs. Use a distribution-native OCCT 8 source build rather than
changing geometry algorithms for packaging.

The full existing license collection is included, with version-matched distro
Qt/PDF/ICU notices under `licenses/linux`. The historical Windows inventory and
Qt 6.11.1 notices do not describe all Linux binaries. `linux-runtime.txt` identifies
the build distribution and library versions. DEB/RPM integrity checks cover installed
files; there is no custom interactive license prompt or license enforcement hook.
Application source archives accompany packages. Dependencies retain their source
and redistribution obligations; the application archive is not their source archive.

Native temporary staging avoids Windows-mount permission artifacts. Build outputs
and package archives remain ignored by Git. Validation covers installation, runtime
resolution, launch and removal in the exact environments recorded in
[Linux package validation](../baseline/linux-package-validation.md).

The `resources/Example` directory is installed at
`/usr/share/foamairplanestudio/Example`. Users should save a copy in their own
folder before editing. Runtime shared libraries use executable permissions so
RPM scans their transitive dependencies; a clean runtime-container install test
checks dependencies not already supplied by the build environment.
