# Panel spars and half-thickness split validation

Date: 2026-09-14. Windows Debug, Qt 6.11.1, OCCT 8.0.0.

Implemented ADR-0012: independent panel tabs/settings, panel-relative lengths,
version-4 persistence and legacy migration, and Mid at half local skin thickness.
Only fixed wing bodies are partitioned; control surfaces bypass panel and height
splits. Overlapping aileron/flap redraws are rejected in the editor. Restored
conflicts remain editable but fail generation before lofting.

Validation:
- project_tests passed (72.03 s): codec/lifecycle, version 4, version-3 migration,
  distinct panel settings, selected tab and malformed panel count.
- spar_panel_tests passed (0.46 s): independent panels, add/remove/restore, units,
  and tab navigation without a data-change callback. Captured panel-spars.png was
  inspected: numbered tabs, units and conditional fields display correctly.
- spar_tests passed (final direct Debug executable run): analytical groove dimensions and removed volumes,
  shifted flat-bottom split with all three spars, chord-dependent varying split
  height continuing beyond hole length, matching tabs/holes, tapered/mirrored
  geometry and two independently configured panels with an intact crossing
  control surface (10 valid solids).
- control_surface_editor_tests passed (0.33 s): click/drag, selection/delete,
  overlap rejection retaining old rectangles, warning on restored overlap,
  cleanup and drawing toggle behavior.
- control_surface_tests passed (15.48 s): both hinges, clearance, volume,
  overlap rejection and mirrored control bodies.

The final spar_tests executable passed after replacing the interim split method.
It now checks hole interiors on each side of the split instead of relying on a
point exactly on the shared boundary. The varying-height fixture also verifies
that the joint continues at its terminal height beyond the physical spar end.

The implemented split uses a chordwise sheet and OCCT Splitter on the uncut main
panel. It verifies two valid solids and adaptive volume conservation, cuts Mid
first, then cuts surface grooves, and finally adds alignment features. It keeps
shared Boolean inputs non-destructive. Final details are in ADR-0012.

Interim closed-cutter approaches failed on the oblique-root shaped-tip workflow:
some returned empty halves and some returned duplicate full wings. Solid count
alone was insufficient; the added volume check caught duplication. Following
the tip's changing height beyond the actual spar was also unreliable. Extending
the spar-end split height to the tip avoids that unnecessary curved joint.
A reverse groove order left an incomplete Mid cut in the flat-bottom test;
Mid-first passed the hole-interior checks. These failed or interrupted diagnostic
runs are not counted as successful validation. No performance claim is made.

The original reference_imgaes/Ailerons_flaps.foam was never modified. SHA-256:
e569dcaa71c5169bdc391e97cd044d4021ea9ef6eea3fb19e4f3e84e47fab21f.
Final full workflow and original-project reproduction results follow below.

Linux/macOS were not tested. Span-region boundaries and finite sampling follow
the conventions/limitations in ADR-0012. Existing app sessions were preserved.


Final station_workflow_tests passed (76.04 s), including all three spar types,
model regeneration, preserved camera, Save, navigation without regeneration or
new dirty state, and readiness reset after deleting a station. The assembled
3D screenshot panel-spar-wing.png was visually inspected. Internal holes/tabs
are verified by the solid tests since the assembled view conceals them.
The final Debug build succeeded; existing OCCT deprecation and Qt deployment
VCINSTALLDIR warnings remain.


Final unchanged Ailerons_flaps.foam reproduction passed: eight valid mirrored
solids with all three spars enabled. Main-wing upper/lower halves are separate;
aileron and flap bodies remain intact. The original file hash remained unchanged.
The rebuilt Debug app launched successfully (PID 21676, FoamAirplaneStudio).
