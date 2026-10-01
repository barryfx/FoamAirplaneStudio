# Windows per-user installer

Build the Windows Release application, then run
`pwsh -File packaging/windows/build-installer.ps1` from the repository. The script
uses Inno Setup 6 and the configured Qt/Visual Studio runtimes, with path overrides
available as parameters. Output is under `dist/`; staging and evidence stay under
`build/installer/`. Neither directory is committed.

Setup targets Windows 10 1809 or later, x64, and installs to the current user's
Local AppData/Programs/FoamAirplaneStudio. `PrivilegesRequired=lowest` prevents
elevation. Start-menu entries and uninstall registration belong to that user;
no services, machine PATH changes, file associations, or system-wide runtime
installations are made. MSVC Release DLLs from the redistributable CRT directory
are deployed beside the executable. Windows supplies UCRT. Installer runtime
updates therefore require a new application installer.
Setup offers an unchecked "Create a desktop shortcut" option. When selected,
it creates a shortcut on the current user's Desktop (including redirected
Desktops), and uninstall removes it. Silent setup can opt in with
`/TASKS="desktopicon"` or opt out with `/TASKS=""`.

Staging uses a fresh directory and deploys Qt Release plugins, OCCT and FreeType,
help and licenses. Test executables, PDBs and debug runtimes are excluded. Optional
D3D/DXC compilers are excluded: the app uses raster Widgets and OCCT OpenGL, and
the old Qt-deployed D3Dcompiler binary had unresolved provenance. The Qt software
OpenGL fallback and its collected notices remain included.

The build verifies every entry in licenses/MANIFEST.json against its SHA-256,
and verifies the application GPL copy matches LICENSE. Setup presents the GPL
and collected third-party text notices for acceptance. All collected license
documents, including RTF/SBOM/provenance files, are installed. Generated installer
code checks every installed license file's SHA-256 before successful completion.
Silent setup requires `/ACCEPTLICENSES=yes`; omission aborts it. This verifies
file integrity and records acceptance, not legal clearance or fulfillment of
corresponding-source distribution obligations. Review the license collection's
distribution notes before publishing binaries.

The stage includes a package-manifest.json with file names, sizes and SHA-256.
Setup is unsigned unless a signing workflow is configured separately. The output
installer's SHA-256 is printed by the build script. No certificate or signing
identity is assumed.

References: [Inno non-admin configuration](https://jrsoftware.org/ishelp/topic_setup_privilegesrequired.htm),
[license page](https://jrsoftware.org/ishelp/topic_setup_licensefile.htm),
[file hashing](https://jrsoftware.org/ishelp/topic_isxfunc_getsha256offile.htm),
[Microsoft app-local deployment](https://learn.microsoft.com/en-us/cpp/windows/choosing-a-deployment-method?view=msvc-170),
[VS 2026 redistribution](https://learn.microsoft.com/en-us/visualstudio/releases/2026/redistribution).

The installer also includes `resources/Example` as `Example` beneath the selected
application directory, containing BabyBuzzard36.foam and its reference PDF.
Use Save As to keep an editable copy outside the installation directory.
