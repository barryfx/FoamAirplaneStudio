# Fuselage thickness and end closure validation
Date: 2026-09-17

Windows Debug build completed for the application and focused Fuselage targets.
No Wing-specific test suites were run. The rebuilt workspace Debug executable
was launched and confirmed responsive.

- `fuselage_profile_tests`: passed (56.41 s), including profile editing,
  persistence and the independent Fuselage worker/cache.
- `fuselage_outline_tests`: passed (2.26 s).
- The new thickness executable passed its UI and persistence checks, rectangular
  wall measurements and open-both/closed-both endpoint checks, then built the
  actual GentleLady fuselage successfully with 8/5/5 mm walls.
- Final `fuselage_thicken_tests`: passed (355.44 s), including all four
  independent nose/tail open/closed combinations.

The thickness regression checks decimal defaults/editing, Side View wing-seat
selection, persistent activation, Save/Open, version-11 migration, invalid-value
rejection and wall metadata retention when sketches move. Pixel checks verify
both assigned profile sketches in Edit Profiles, Thicken and Cut. The fixture is
explicitly framed in the viewport before pixel inspection.

Solid point classification measures 8 mm, 6.5 mm and 5 mm inward wall offsets at
three longitudinal positions and distinguishes cavities from retained end walls.
The builder checks OCCT validity and positive material volume below the exterior
volume. End fixtures cover openings at profile endpoints and closed extensions.
Thickness is measured in section planes; this is not a global surface-normal
manufacturing tolerance certification.

GentleLady was read without modifying the file. SHA-256 remains
`9C3C7B01E53B7CF274FD5292D315AA74F6A1DD1143EC245C7CD43724AC99D67C`.
Its native OCCT screenshot was inspected: upright profiles, guided contours,
closed extended ends and no visible nose twist. A closed exterior naturally
hides the inner cavity in this view; solid checks validate the hollow geometry.

Ignored local evidence:
- `build/fuselage-final-build.log`
- `build/fuselage-thicken-tests.log` (initial run and visibility-test setup issue)
- `build/fuselage-thicken-final-tests.log`
- `build/gentlelady-thickened.log`
- `build/debug/gentlelady-thickened.png`
- `build/debug/fuselage-thicken.png`

An initial pixel check sampled outside the visible drawing area; framing the
fixture corrected the test. Deferred Qt layout events are drained before checking
newly rebuilt field visibility. One incremental linker run reported an invalid
COFF object; rebuilding the affected geometry translation unit resolved it.

## Reference units and station ordering follow-up

The focused `fuselage_thicken_ui_tests` passed in 1.34 s. Three stations are
created out of longitudinal order: both forward of the Side View wing seat
receive 8 mm, while the aft station receives 5 mm. The panel lists Station 1,
Station 2 and Station 3 in that order. Checks cover Reference-unit restoration
and changes, bare decimal entry in both units, explicit mm/in overrides, invalid
input rejection, physical-value persistence and profile visibility. The captured
panel (`build/debug/fuselage-thicken-units.png`) was visually inspected.

Application and test targets rebuilt in Debug. Geometry generation was unchanged;
the expensive geometry cases above were not repeated. No Wing-specific tests ran.
Build/test logs: `build/fuselage-units-build.log` and
`build/fuselage-units-tests.log`.

## Nose-to-tail numbering correction

GentleLady version 11 stores station X coordinates in order 256.3825, 71.5530,
465.7656, with the confirmed LE at 253.1733. Creation-order labels therefore
called an aft-of-LE station Station 1 and the forward station Station 2. Display
numbers now follow ascending Side View X in both Thicken and Edit Profiles.
Only display order changes; saved station records, profile slots and thickness
assignments stay intact. This supersedes the creation-order numbering described
in the previous follow-up.

`fuselage_thicken_ui_tests` passed in 1.39 s with explicit row-to-station mapping
and matching Edit Profiles selection labels for stations created out of order.
The captured panel was visually inspected. Debug rebuilt and relaunched; no
Wing-specific tests ran. Logs: `build/fuselage-numbering-build.log` and
`build/fuselage-numbering-tests.log`; capture: `build/debug/fuselage-numbering.png`.

## Inclusive leading-edge defaults

The default boundary is now inclusive: 8 mm at or forward of the LE, 5 mm aft.
A detected Side View seat registers its nearest station within 2% of the root
chord as the LE station, allowing the small tracing offset in GentleLady.
The focused UI test passed in 3.02 s, covering an exact LE station, a near-LE
station, an aft station outside tolerance, and the actual GentleLady file loaded
through MainWindow. GentleLady initializes to 8/8/5 mm in nose-to-tail order.
The source file was not saved. Existing entered thickness values remain intact.
An initial test run stopped at an unsaved temporary-fixture prompt; the test now
saves its temporary fixture before each Open and passes. Geometry was unchanged;
no Wing-specific tests were run. Debug rebuilt and relaunched.
Logs: `build/fuselage-le-build-final.log`, `build/fuselage-le-tests.log`.
