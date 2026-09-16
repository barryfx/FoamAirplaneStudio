# Lightening validation
Date: 2026-09-15
Platform: Windows, Qt 6.11.1, OCCT 8, Debug

## Implemented behavior
Whole-wing enable flag, wall and crossmember thicknesses, uniform crossmember
count, first-root start distance and final-station stop distance. All physical
fields preserve mixed-unit entry text. Only the main wing is hollowed and split;
controls remain solid and intact. Panel caps and the final tip remain solid.
Existing Mid pin/socket locations retain full-height support with socket radius
plus wall thickness. All settings participate in version-8 persistence and dirty
tracking; versions 1-7 initialize the feature disabled.

## Automated checks
All relevant suites passed:

| Suite | Last relevant passing run |
| --- | --- |
| lightening_tests | 166.44 s |
| project_tests | 77.90 s |
| spar_tests | 106.77 s |
| wing_solid_tests | 197.63 s |
| spar_panel_tests | 0.42 s |
| reference_tests | 0.39 s |

Lightening tests cover skins/LE/TE/end material, uniform ribs across two panels,
solid panel joints and final-station cutoff, mirrored cavities, mandatory access
splits, grooves/Mid holes and alignment support, the original 0.1 mm socket fit,
untouched appended controls, mm/in fields, invalid input, old-file defaults and
invalid spacing. A spanwise intersection test rejects extra internal pocket
partitions. A cap-boundary regression ensures a pocket starting before a panel
root does not lose an entire construction slice. Project UI tests cover Save,
Open, New reset and clean/dirty behavior for Lightening settings.

The generic solid classifier was ambiguous for some rays through stepped edges;
known-direction surface intersection intervals verify the tested material and
socket gap. Cached cutaway geometry was also reopened and checked below the split:
cavities at spans 40, 90, 140 and 200 mm; retained ribs at 62.5, 115 and 167.5 mm.

## Actual-project and visual checks
The app's project-open path successfully loaded and generated
`build/debug/lightening-smoke.foam`, a separate copy of the prior GentleLady
validation fixture with 3/5-degree dihedral, original surface spars, 2 mm wall,
3 mm ribs, four crossmembers and 25 mm root/final-station setbacks. The original
user project was not written. The smoke asserts wingModelReady and no unsaved
input changes after opening/generation/display.

Inspected images:
- `build/debug/lightening-gentle-lady.png`: actual data panel and assembled wing.
- `build/debug/lightening-gentle-lady.png.front.png`: retained dihedral silhouette.
- `build/debug/lightening-cutaway-final.png`: lower halves of the two-panel test
  wing, three requested crossmembers plus solid panel joint and tip, open stepped
  pockets, and no extra construction-slice partitions.
- `build/debug/lightening-cutaway.png.panel.png`: mixed-unit field presentation.

The cutaway is a validation-only isolated-half view. The application continues to
show all generated bodies assembled. Screenshots wait for the native viewport to
paint; a top view with hidden face edges does not reveal flat pocket depths.
The saved cutaway also renders correctly with its original generated mesh.

## Resolved checks and limits
A whole-solid inward offset failed on the GentleLady shape. The implemented
conservative envelope retains thin material instead. Initial touching-prism tools
left unwanted internal faces; the final tools are fused and simplified before
subtraction, with a regression that checks for these faces. The final Debug app
and all test targets build successfully. Existing OCCT deprecation and Qt deployment
VCINSTALLDIR warnings are nonfatal.

Pockets deliberately have stepped floors/ceilings and may retain more than the
minimum wall. Generation remains synchronous and large wings can take minutes;
no interactive performance or structural-strength claim is made. Source sketches
and original project files remain intact. See ADR-0017 and architecture/lightening.md.
