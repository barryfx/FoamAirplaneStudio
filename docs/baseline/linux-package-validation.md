# Release installer validation — 2026-10-01

All three distributable installers use Release executables. A separate Windows
Debug rebuild/launch satisfied the repository's implementation check; it was not
used in any installer. No Wing or Fuselage generation tests were run.

## Ubuntu 24.04.3 LTS, x86-64

Built the DEB under WSL 2 with GCC 13, Qt 6.4.2 (including PDF) and OCCT 8.0.0.
APT installation and removal succeeded. `dpkg -V` reported no changed package
files. Installed application and XCB/JPEG plugin dependencies resolved. The
installed launcher opened the main window under WSLg; the smoke test checked
its live process and loaded private Qt Core/PDF and OCCT libraries. Removal
removed the launcher and desktop-menu entry.

Three selected Debug editor tests passed: `file_dialog_tests`,
`sketch_editor_tests`, and `reference_tests` (1.38 seconds total). The first run
could not locate transitive custom OCCT libraries in two test executables; the
rerun passed with the documented OCCT `LD_LIBRARY_PATH`. The installed launcher
sets its own private runtime path.

Evidence: `build/linux-deb-final.log`, `build/ubuntu-package-install-final.log`,
`build/ubuntu-installed-smoke-final.log`, `build/ubuntu-package-remove-final.log`,
and `build/linux-editor-checks-rerun.log` (local ignored build artifacts).

## Fedora 44, x86-64

Built the RPM with GCC 16.1.1, Qt 6.11.2/PDF and a native OCCT 8.0.0 Release
source build. Fedora's system OCCT 7.9.3 cannot compile the application's OCCT 8
APIs. OCCT was built from the upstream V8_0_0 archive on the container's native
filesystem, avoiding slow Windows bind-mount header scanning.

A separate Fedora 44 runtime container installed the RPM with DNF. Initial
validation exposed three missing transitive Qt dependencies (libb2, pcre2-utf16,
and double-conversion). Installing bundled ELF libraries with executable
permissions enabled RPM's automatic dependency scanner to detect them. The
rebuilt RPM installed those dependencies automatically. `rpm -V` reported no
changed files. The app launched as a normal user under Xvfb, with private Qt
Core/PDF and OCCT libraries loaded and XCB/JPEG plugin dependencies resolved.
DNF removal succeeded and removed the launcher and installed examples.

Evidence: `build/fedora-occt-native.log`, `build/linux-rpm-examples.log`,
`build/fedora-package-install-final.log`, `build/fedora-package-verify-final.log`,
`build/fedora-package-smoke-final.log`, and `build/fedora-package-remove-final.log`.

## Windows x64

Rebuilt the Release app and installer. The refreshed installer installed to a
temporary workspace directory, verified all license hashes, matched all 172
package-manifest file hashes, and launched a responsive installed application.
The test installation was uninstalled. The existing Start-menu shortcuts were backed up and restored byte-for-byte for
the final example-bearing installer check; the pre-existing application and desktop
shortcut were retained. The existing installed application was left running.

Evidence: `build/linux-packaging-windows-release.log`,
`build/windows-installer-examples.log`, `build/windows-install-smoke-examples.log`,
and `build/windows-install-result-examples.txt`.

## Included example and final artifacts

Rebuilt all three Release installers after adding `resources/Example`.
Both `BabyBuzzard36.foam` and `Baby_Buzzard_Plan_559_New.pdf` matched their source
SHA-256 hashes after installation on Windows, Ubuntu and Fedora. Windows installs
`Example` below the application directory; Linux uses
`/usr/share/foamairplanestudio/Example`. Example geometry was not regenerated.

The final Ubuntu installation was left installed for use. Its final checks are
`build/ubuntu-package-install-examples.log` and
`build/ubuntu-installed-smoke-examples.log`; removal was already verified with
the preceding package. Windows and Fedora temporary test installs were removed.
The project-required Windows Debug rebuild and launch also completed.

## Scope

Linux checks cover packaging, dependency resolution, installed-file integrity,
startup and removal. Ubuntu was tested with WSLg; Fedora uses a virtual X11
display in a container. Native Linux desktop interaction, other distributions,
macOS and geometry generation are not claimed as tested here. SHA-256 files in
`dist/` identify the generated packages and application-source archives.
