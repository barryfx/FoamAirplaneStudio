# Version 0.2.0 installer validation — 2026-10-04

Built all three Release installers from source tag `v0.2.0`, with the license
manifest correction below. Build commands use eight parallel jobs. Artifacts and
SHA-256 files are in ignored `dist/`; they have not been published to GitHub Releases.
The existing source tag is unchanged.

## Packaging correction

The Windows packager rejected five stale license-manifest hashes. All five
matched the checked-in text exactly after LF-to-CRLF conversion, confirming
that Git's LF normalization, not changed license wording, caused the mismatch.
Updated `qt-pdf-source.txt` and four `graphics/dxc` manifest hashes to describe
the repository's actual LF bytes. Source archive checksums remain unchanged.
The packager's strict hash verification remains enabled. All manifest entries
were revalidated; both Linux packages were rebuilt with the corrected manifest.

## Windows x64

Release build and Inno Setup compilation passed. An isolated per-user install
passed all 172 package-manifest file hashes, including both bundled example
files. The installed app launched and remained responsive. Uninstall returned
zero and removed its registration; pre-existing executable and shortcut hashes
were preserved. Silent setup without license acceptance returned 1 and installed
no application. Installer is unsigned. This is not a clean Windows VM test.

Evidence: `build/package-020-windows.log`, `build/windows-install-result-020.txt`,
`build/windows-install-smoke-020.log`, `build/package-020-windows-license-test.txt`.

## Ubuntu 24.04 x86-64

Built under Ubuntu WSL with GCC 13, Qt 6.4.2 and OCCT 8.0.0. Installed with APT
in a fresh Ubuntu 24.04 runtime container. Enabled installation of this package's
documentation because the container image excludes most `/usr/share/doc` files
by default. Final `dpkg -V` output was empty. Application, XCB and JPEG plugin
dependencies resolved. Normal-user launch under Xvfb showed the main window and
loaded private Qt and OCCT libraries. Example files and the corrected manifest
matched repository bytes. Removal eliminated the launcher, desktop entry and
example. The existing Ubuntu WSL installation was not replaced.

Evidence: `build/package-020-deb-final.log`, `build/package-020-deb-test-final.log`,
`build/package-020-deb-remove.log`.

## Fedora 44 x86-64

Built with native Fedora Qt 6.11.2 and OCCT 8.0.0 in the existing build container.
DNF installed the RPM in a fresh Fedora 44 runtime container; `rpm -V` output
was empty. Application, XCB and JPEG plugin dependencies resolved. Normal-user
launch under Xvfb showed the main window and loaded bundled Qt/OCCT libraries.
Both examples and the manifest matched repository bytes. Removal eliminated
the launcher, desktop entry and example.

Evidence: `build/package-020-rpm-final.log`, `build/package-020-rpm-test-final.log`,
`build/package-020-rpm-remove.log`.

## Scope

These are packaging, installed-file, startup and removal tests. Geometry was not
regenerated; Linux native desktop interaction and macOS were not tested.
Linux application-source archives accompany the packages, and all generated
artifact SHA-256 files were checked. The archives include the corrected license
manifest; this subsequently written validation record is not part of those archives.
