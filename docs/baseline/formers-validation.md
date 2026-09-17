# Formers validation
Date: 2026-09-17

Windows Debug initial focused tests passed:
- `former_tests`: 65.61 s.
- `former_geometry_tests`: 0.69 s.

Geometry tests verify full-height cavity intersection, partial height, zero
positive-volume overlap with the skin/tray, support-ledge clearance, tapered
walls, collision rejection, missing cavity/outside placement errors and cancellation.
The UI test uses actual Qt mouse events to add formers, enter mm/in thickness,
move in both axes, resize both ends, reject collisions, delete and recreate.
It verifies saved rectangles, version-14 migration, legacy Firewall tab migration,
malformed overlapping data rejection and persistent green overlays outside Formers.
The panel screenshot was visually inspected. Full generation produces four
separate bodies (supported fuselage, tray and two formers), reports two formers,
reuses the unchanged cache and leaves Wing model revision zero.

Evidence (ignored local artifacts): `build/formers-build.log`,
`build/formers-test-build.log`, `build/formers-tests.log`,
`build/formers-final-build.log` and `build/debug/formers.png`.

These checks cover representative rectangular/tapered cavities. Masks that miss
the cavity or produce disconnected pieces report a generation error. Collision
editing uses conservative 2D masks; touching edges are permitted. Cut remains
applied to the supported fuselage, while formers and tray are separate inserts.
No Wing-specific tests were run. No user project was overwritten.

Final Debug rebuild succeeded. The final focused run passed 3/3 in 119.64 s:
`former_tests` 64.85 s, `former_geometry_tests` 0.69 s, and
`servo_tray_tests` 54.03 s. See `build/formers-final-tests.log`.
The rebuilt Debug application was launched.

## Default thickness revision

GentleLady contained valid tray/former dimensions but had never enabled Thicken,
leaving station wall values null. Insert regeneration now initializes those wall
defaults and enables hollowing, preserving explicit station values. The default
3 mm insert dimensions require no field edits. Zero dimensions are rejected.

Validation passed: `former_defaults_tests` (247.70 s) regenerates untouched
3 mm tray/former defaults without visiting Thicken, preserves an explicit 4 mm
station wall, fills a missing wall, and saves the initialized state. The model
contains three bodies with no Wing regeneration. `former_geometry_tests` (0.65 s)
also verifies explicit zero tray/former rejection. Evidence:
`build/insert-defaults-tests.log`, `build/insert-defaults-build.log` and
`build/insert-defaults-final-build.log`. Debug rebuilt and launched successfully;
no Wing-specific tests ran and no user project was overwritten.
