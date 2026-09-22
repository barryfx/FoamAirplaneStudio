# Circle profile validation

Date: 2026-09-21. Windows Debug.

Built `designrc`, `fuselage_profile_tests`, `sketch_editor_tests`, and
`fuselage_outline_tests`. Ran only `circle_profile_tests` (the profile test's
non-generation mode), `sketch_editor_tests`, and `fuselage_outline_tests`.
All three passed in 5.45 seconds total. No aircraft generation suites ran.

Circle checks cover button placement and tool exclusivity, center/radius clicks,
zero-radius rejection, Escape and tool-switch cancellation, pending-circle
save/open, moving the center without changing radius, resizing, circular sampled
boundary, selection/Delete, format-28 round-trip, old-format migration, invalid
radius rejection, and rejection of circles in unsupported sketch collections.

Inspected `build/debug/circle-profile.png`: Circle is below Line/Spline, the
station remains selected, and the circular profile has center and radius handles.
Formers received no drawing changes. Build/test logs are in
`build/circle-profile-build.log` and `build/circle-profile-tests.log`.
Linux/macOS and generated fuselage solids were not exercised in this validation.
