# License collection validation

Date: 2026-09-25

Added a top-level license collection with 59 Qt component notice files,
application GPL text, Qt/OCCT/FreeType terms, Mesa/LLVM notices, and supplemental
Microsoft SDK/DXC terms. The collection has 82 files, including its manifest,
component index and the inspected Windows Debug DLL inventory. Qt PDF notices
come from the exact official 6.11.1 documentation archive, verified against its
published SHA-1. Other Qt notices come from the matching installed documentation.
All 78 upstream files in MANIFEST.json passed SHA-256 verification. The GPL copy
matches the repository LICENSE byte for byte. All local component-index links
resolve. All 82 files match their deployed copies byte for byte.

Changed Windows post-build deployment to copy the collection; the old resources
license texts remain preserved as provenance. Built Debug designrc successfully
(build/debug/license-deployment-build.log). Reran only startup_gui_tests: passed
in 5.17 seconds, with no model-generation tests. Launched the rebuilt Debug app.

No incompatible selected open-source license path was identified. The collection
README records license alternatives, scope, distribution obligations and the
unresolved provenance of Qt's older D3Dcompiler_47.dll. This is not a complete
legal clearance of a release installer. No application license was changed.
