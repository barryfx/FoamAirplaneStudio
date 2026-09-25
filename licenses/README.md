# FoamAirplaneStudio licenses

Reviewed: 2026-09-25. Application license: **GPL-3.0-only**.

This folder collects the application license and the upstream license/copyright
notices for the current Qt 6.11.1, OCCT 8.0.0 and FreeType Windows dependency set.
The Windows post-build deployment copies this folder to `licenses` beside the
application. Original dependency notices are preserved; collecting them does not
relicense their code or provide the corresponding source required on distribution.

## Contents and scope

- `GPL-3.0.txt`: byte-identical copy of the repository's `LICENSE`.
- `QT-LGPL-3.0.txt`: selected Qt license; read together with GPL-3.0.
- `OCCT-LGPL-2.1.txt` and `OCCT-LGPL-EXCEPTION.txt`: OCCT terms.
- `FREETYPE-LICENSE.txt` and `FREETYPE-FTL.txt`: standalone FreeType 2.13.3
  used by OCCT. Qt's separate embedded FreeType version has its own notice below.
- [COMPONENTS.md](COMPONENTS.md): index of 59 third-party notices from the Core,
  GUI, Network, SVG and PDF module lists. `qt/` contains their full license and
  copyright blocks, extracted from matching Qt 6.11.1 documentation. This is a
  conservative module-level superset including platform-conditional components,
  not a claim that all are compiled into the Windows application.
- `graphics/`: Mesa/llvmpipe and LLVM notices from Qt; DXC release terms as
  supplemental provenance. See Microsoft qualification below.
- `ICU-LICENSE.txt`: historical ICU 74 packaging notice. The current Qt PDF ICU
  notice is separately retained in `qt/qt-pdf/qtpdf-3rdparty-icu.txt`.
- `qt-sbom/`: unmodified upstream SBOMs for QtBase, QtSvg and QtPdf. These include
  SDK tools and unused modules, and are not an application dependency inventory.
- `MANIFEST.json`: source and SHA-256 for each collected upstream notice/SBOM.
- `qt-pdf-source.txt`: official Qt PDF **6.11.1** documentation archive and its
  verified SHA-1 plus locally computed SHA-256. No newer online documentation was
  substituted for this module's license texts.
- `windows-runtime-inventory.json`: DLL names and hashes observed in Debug,
  including plugin subdirectories. This is a snapshot, not a release manifest.

The configured OCCT build enables FreeType and OpenGL and disables TBB, FreeImage,
VTK, FFmpeg and Tcl. Unrelated libraries in the sibling third_party directory are
not dependencies merely because they are installed. The current deployment has
Schannel/certificate-only TLS plugins and no OpenSSL DLLs.

## Compatibility assessment

No incompatible open-source license requirement was identified in the selected
license paths for the inspected application and dependency notices:

| Component or family | Selected terms / compatibility considerations |
|---|---|
| Application | GPL-3.0-only; unchanged |
| Qt runtime modules | LGPL-3.0; preserve replaceability and applicable source/relinking rights |
| OCCT | LGPL-2.1 with OCCT exception; preserve notices and library source rights |
| FreeType and Qt's FreeType rasterizer | **FTL**, not the GPL-2.0-only alternative |
| Abseil, Tika definitions, other Apache code | Apache-2.0 is GPLv3-compatible; preserve any notices |
| zlib, PNG, JPEG, PCRE2, HarfBuzz, TinyCBOR, MD4C, PDFium and similar code | Permissive terms; preserve the component-specific notices, not just a generic MIT/BSD text |
| Public Suffix List | MPL-2.0; preserve its file-level notices and source obligations |
| Unicode data, fonts and color profiles | Their own permission notices remain applicable; retain attribution and any naming/modification conditions |
| Mesa / LLVM | Permissive terms recorded in their supplied notices |

A GPL-2.0-only dependency without an alternative or exception would conflict
with GPL-3.0-only. FreeType offers FTL as the compatible alternative; the presence
of GPLv2 text in its upstream dual-license notice does not select that option.
Similarly, copying Qt's SBOM does not select commercial Qt licensing or the GPLv2
alternative. Complete notice blocks may retain unselected upstream alternatives.

This is an engineering inventory and compatibility assessment, not legal sign-off
for a particular installer. Source-code delivery and other distribution duties
still apply; a folder of license texts alone does not fulfill them.

Authoritative compatibility references:

- https://www.gnu.org/licenses/license-compatibility.en.html
- https://www.gnu.org/licenses/license-list.html
- https://apache.org/licenses/GPL-compatibility.html
- https://doc.qt.io/qt-6/qtpdf-licensing.html

## Microsoft binaries and remaining release check

Microsoft graphics/runtime binaries are not relicensed under the application's
GPL. The installed Windows SDK 10.0.26100.0 license and third-party notices are
included verbatim under `microsoft/windows-sdk-10.0.26100.0/`.
The deployed `dxcompiler.dll` and `dxil.dll` (1.8.2502.11) are byte-identical to
that SDK's x64 bin copies. They are **not** identical to GitHub DXC v1.8.2502
binaries: the latter's MIT/LLVM/Microsoft/HLSL license texts are supplemental
references in `graphics/dxc/`, not proof of the exact SDK binaries' licensing.

The deployed `D3Dcompiler_47.dll` reports 6.3.9600.16384 and comes from Qt's runtime
deployment. It did not match the installed Windows 10 SDK copies. Its original
Windows 8.1-era distribution/license provenance remains to be confirmed before
publishing an installer. The SDK reference terms do not establish that match.
Microsoft's distribution restrictions require a separate review; do not treat
this open-source compatibility assessment as clearance for all Windows DLLs.

The current build is Debug and intended for local development. A release audit
must inventory the actual Release binaries, Microsoft redistributables, plugins,
source availability and version-matched notices. Linux/macOS releases need their
own platform inventories. Do not infer licensing obligations from unused build
tools or optional modules appearing in the full Qt SDK SBOM.

## Updating

Recollect notices whenever a dependency version or enabled feature changes.
Use that version's supplied notices and SBOM, preserve copyright text and full
license terms, and update the source/hash manifest. Do not silently replace a
version-pinned notice with documentation for a newer patch release.
`resources/licenses/` retains the earlier upstream source copies for provenance;
`licenses/` is now the deployed collection.
