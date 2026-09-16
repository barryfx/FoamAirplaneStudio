# Ailerons_flaps spar failure investigation
Date: 2026-09-14

The source project is reference_imgaes/Ailerons_flaps.foam (the actual folder
name is misspelled). No project data was edited.

The saved AG35 profile has an almost flat lower surface at chord height zero.
Settings: Top round 2.54 mm diameter at 30% chord / 70% span; Bottom strip
6.35 mm wide by 2.54 mm deep at 30% / 70%; Mid round 5.08 mm at 30% / 60%.

A read-only reproduction with the full saved outline, stations, airfoils and
control cuts completed the Top and Bottom cuts, then failed Mid at 0% span.
The generated solid provides 15.824 mm above the chord plane and effectively
0 mm below; the centered hole needs 2.540 mm on each side. This is a placement
constraint, not insufficient total wing thickness. A different split/hole plane
would be a design change; the diagnostic fix preserves the requested chord plane.

All spar construction errors now identify Top/Bottom/Mid. Thickness errors give
span distance/percentage and required/available physical dimensions. Mid split
and alignment errors are also labeled. Regression tests use a flat-bottom solid
with ample total thickness, verify valid surface grooves, and require a detailed
Mid error. Oversized Top and Bottom errors are checked separately.

Debug build succeeded after adding the project reader's visualization link
libraries to the manual test target. The initial linker failure prevented that
test executable from running; after the dependency correction, spar_tests passed
(28.75 s). It includes existing groove, split, alignment, taper and mirror checks.

Reproduction commands (from project root):
```
build/debug/Debug/spar_tests.exe reference_imgaes/Ailerons_flaps.foam
build/debug/Debug/spar_tests.exe reference_imgaes/Ailerons_flaps.foam --surface-only
```
The second command changes only the in-memory Mid flag. The original file's
SHA256 is e569dcaa71c5169bdc391e97cd044d4021ea9ef6eea3fb19e4f3e84e47fab21f.

Surface-only reproduction completed successfully with six valid mirrored solids (fixed wing, aileron and flap on each side). The source project hash remained unchanged. Debug rebuilt and launched successfully. Help HTML nesting and updated documentation links were checked. The earlier documentation audit was also completed for current feature descriptions, project versions, control selection/clearance, spar requirements and Help.

The later half-thickness split in ADR-0012 supersedes the chord-plane behavior investigated here. See panel-spar-validation.md for the new reproduction result.
