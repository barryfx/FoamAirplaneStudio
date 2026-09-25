# Assembly validation — 2026-09-19

## Former wing seats — 2026-09-23
The Debug build, `assembly_tests` (10.55 s) and `export_tests` (7.30 s) passed.
Synthetic solids cover wing-seat volume, renamed former identity and section
plane preservation, unchanged source caches/tray, no stabilizer cuts in formers,
rotated and translated wing placement, disconnected valid remnants, rejection of
complete consumption, cancellation at the former-cut stage, GUI Undo Cuts, and
the cut former in exported geometry. These checks do not generate a Wing or
Fuselage model. Evidence: `build/debug/airfoil-former-tests.log`.

Windows Debug, branch Assembly.

- Configured with `cmake --preset windows-debug`.
- Built `designrc`, `assembly_tests` and `stabilizer_outline_tests` with the
  windows-debug build preset. Final application and Assembly test rebuild passed.
- `ctest --preset windows-debug -R '^assembly_tests$' --timeout 120`: passed,
  final run 4.71 seconds. Covers box-based saddle/pass-through/fin cuts, valid
  remaining solids, untouched originals, named control collisions, cancellation,
  Assembly entry and 2D lockout, keyboard movement, disabled selectors after cuts,
  export snapshot selection, undo, source invalidation, format-21 persistence and
  older-version defaults. Cached synthetic parts avoid Wing/Fuselage generation.
- `ctest --preset windows-debug -R '^stabilizer_outline_tests$' --timeout 600`:
  passed in 230.58 seconds after preserving fixed/control geometry roles.
- Reference alignment checks cover manual scale and physical drawing units.
  Inspected `build/debug/assembly-cut.png`: upright reference text, red image
  outline aligned around the synthetic fuselage, aircraft visible in front,
  2D disabled and Undo Cuts visible. The red outline belongs to the raster test
  image; no sketch presentations are included. Image pages use the 2D stacking
  and physical-size rules, and camera fitting excludes the reference pages.
- `git diff --check`: passed.

Wing and Fuselage generation tests were not run, per AGENTS.md. The full real
project preparation path generating those missing components was not exercised.
Actual STEP/STL/DXF/SVG export remains future work; the Assembly export accessor
is tested to return cut geometry when cuts are active. Collision checks concern
resting solids rather than control-surface deflection envelopes.

The rebuilt Debug application was launched and confirmed responding with the
window title 'Untitled - FoamAirplaneStudio'.

## Open in 2D follow-up

Project restoration now forces 2D; saved Assembly workspaces open in Fuselage
Side View. Regression coverage opens files saved from workspaces 0–5 in both
viewport modes, checks no model jobs or generated Wing/Fuselage shapes, verifies
preserved Assembly cut intent/offsets, and confirms a clean document. The Assembly
suite passed in 7.05 seconds. The Debug application, Assembly tests and project
test executable were rebuilt. Project generation tests were updated to explicitly
select 3D where required, but that broader suite was not run under the generation
restriction in AGENTS.md.

## Concurrent preparation follow-up

Assembly now runs missing component builders through the existing bounded task
scheduler. Windows Debug builds of designrc, assembly_tests and processing_tests
passed. The targeted suites passed: Assembly 6.53 seconds, processing 0.07 seconds.
Processing coverage synchronizes four independent tasks at a barrier to prove
concurrent execution and checks independent result slots; existing tests cover
bounded concurrency, cancellation and sibling joining after failure. Assembly
coverage reuses cached synthetic models and checks lifecycle/cut behavior.

Actual concurrent Wing/Fuselage generation and end-to-end timing benchmarks were
not run because AGENTS.md prohibits those generation tests. No measured model
speedup is claimed. The four-worker bound covers component tasks; existing OCCT
internal meshing parallelism is unchanged. Wing panel workers are limited to one
when other missing components are scheduled.

## Root rotation and angle display (2026-09-23)

Assembly adds Clockwise/Counter-Clockwise buttons and an angle readout directly
below them. Clicks change the selected component by +/-0.5 degrees about the root
chord midpoint. Source-outline root pivots are independent of sweep/cuts; the
attached elevator/rudder shares its fixed component transform. Positive is
clockwise in the standard negative-Y side view. Placements use immutable source
shapes, preserve translations, and feed cut/export and Weight and Balance.

Windows Debug validation:
- assembly_tests passed (11.10 s): transformed vertices match independent sine/
  cosine expectations at +0.5, -0.5 and +1 degrees; translated pivots stay fixed,
  paired controls follow, and source solids/volumes remain unchanged. GUI clicks
  verify half-degree changes and selected angle text, undo/redo, save/read,
  disabled rotation after cuts and default-zero/malformed-angle decoding.
- project_persistence_ui_tests passed (1.54 s).
- weight_balance_tests passed (2.28 s), including rotation cache invalidation.
- Panel capture build/debug/assembly-rotation.png visually checked: both buttons
  and Rotation: +0.5 degrees are visible directly beneath the selectors. Qt widget
  capture does not capture the native OCCT viewport; shape motion is covered by
  geometric coordinate assertions.
- Debug rebuilt and launched. No Wing or Fuselage generation tests were run.
