# FoamAirplaneStudio Third-Party Notices

FoamAirplaneStudio is Copyright (C) 2026 Barry Foust and is licensed under the GNU General Public License
version 3 only (`GPL-3.0-only`). See `LICENSE`.

FoamAirplaneStudio dynamically links the following separately licensed libraries. The distributed DLLs must
remain replaceable by compatible, user-modified builds. Distribution terms must not prohibit
reverse engineering when it is necessary to debug modifications to these libraries.

The current Windows build deploys Qt, OCCT and FreeType. Linux package details
below are retained historical dependency notices; current Linux release packaging
has not been validated. No dependency sources were modified.

## Qt 6

FoamAirplaneStudio uses Qt Core, GUI, Widgets, OpenGL support, and platform runtime plugins. Windows releases
bundle Qt 6.11.1. Historical Ubuntu 24.04 packaging bundled Qt 6.4.2 from Ubuntu's `qt6-base` package. Qt is
Copyright (C) The Qt Company Ltd. and other contributors. The Qt components used by FoamAirplaneStudio are
distributed under the GNU Lesser General Public License version 3 (`LGPL-3.0-only`). FoamAirplaneStudio does
not modify Qt.

- Project: https://www.qt.io/
- Windows QtBase source: https://code.qt.io/cgit/qt/qtbase.git/tag/?h=v6.11.1
- Ubuntu QtBase source package: https://packages.ubuntu.com/source/noble/qt6-base
- License: `licenses/QT-LGPL-3.0.txt`
- GNU GPL version 3 incorporated by the LGPL: `LICENSE` in the source repository (also included with deployed license notices)

Qt contains third-party components under their own licenses. The exact components deployed with a
release are documented by Qt's generated SBOM. The build copies the QtBase and QtSvg SPDX documents
to `licenses/qt-sbom` and Qt's relevant attribution pages to `licenses/qt-attributions`.

The Windows deployment includes `opengl32sw.dll`, the Mesa llvmpipe software OpenGL renderer,
under its permissive licenses. Its complete Qt-supplied attribution is installed as
`licenses/qt-attributions/qt-attribution-llvmpipe.html`. `D3Dcompiler_47.dll`, `dxcompiler.dll`,
and `dxil.dll` are Microsoft redistributable components collected by `windeployqt` and remain under
Microsoft's applicable terms.

## Open CASCADE Technology 8.0.0

FoamAirplaneStudio uses Open CASCADE Technology (OCCT), Copyright (C) 1999-2026 OPEN CASCADE S.A.S. OCCT is
distributed under the GNU Lesser General Public License version 2.1 with the Open CASCADE exception.
FoamAirplaneStudio does not modify OCCT.

- Project: https://dev.opencascade.org/
- Exact source: https://github.com/Open-Cascade-SAS/OCCT/tree/V8_0_0
- License: `licenses/OCCT-LGPL-2.1.txt`
- Exception: `licenses/OCCT-LGPL-EXCEPTION.txt`

## FreeType

Windows releases distribute FreeType 2.13.3 for OCCT font support. Historical Ubuntu packaging used the
system-provided FreeType 2.13.2 shared library instead of bundling it. FreeType is Copyright (C)
1996-2024 David Turner, Robert Wilhelm, Werner Lemberg, and other contributors. The Windows
distribution uses the FreeType License (FTL), a BSD-style license with an attribution requirement.
FoamAirplaneStudio does not modify FreeType.

- Project: https://freetype.org/
- Exact source: https://download.savannah.gnu.org/releases/freetype/freetype-2.13.3.tar.xz
- License overview: `licenses/FREETYPE-LICENSE.txt`
- FreeType License: `licenses/FREETYPE-FTL.txt`

## International Components for Unicode 74

Historical Debian/Ubuntu packaging bundled the ICU 74 runtime libraries required
by Qt 6.4.2 for compatibility with Debian releases providing a different ICU ABI.
ICU is Copyright (C) 1991-2022 Unicode, Inc. and other contributors and is distributed under its
permissive license. FoamAirplaneStudio does not modify ICU.

- Project: https://icu.unicode.org/
- Exact source package: https://packages.ubuntu.com/source/noble/icu
- License: `licenses/ICU-LICENSE.txt`

Portions of Qt and OCCT may use additional third-party components. Before publishing an installer,
compare its complete DLL and plugin inventory with the packaged Qt and OCCT SBOM/license data and
add any required notices. Microsoft runtime components, when included, are redistributed under
Microsoft's applicable terms and are not covered by FoamAirplaneStudio's GPL license.


## Qt PDF

Reference PDF rendering uses Qt PDF 6.11.1 (QtPdf), dynamically linked under the
same Qt LGPL-3.0 terms above. Qt PDF embeds PDFium and associated third-party
components. The add-on's SPDX bill of materials is copied to licenses/qt-sbom.
Source and attribution references:
- https://doc.qt.io/qt-6/qtpdf-index.html
- https://code.qt.io/cgit/qt/qtwebengine.git/tag/?h=v6.11.1
- https://doc.qt.io/qt-6/qtwebengine-licensing.html

The matching Windows add-on was obtained from Qt's official repository:
https://download.qt.io/online/qtsdkrepository/windows_x86/extensions/qtpdf/6111/msvc2022_64/
