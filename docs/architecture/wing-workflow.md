# Wing toolbar progression

The Wing workspace itself remains disabled until Reference prerequisites are
satisfied (see reference.md). The following rules govern its secondary toolbar
after Wing is unlocked.

The Wing workspace initially selects/enables Outline; every other secondary
action is disabled. Definition state unlocks actions cumulatively:

| Definitions present | Enabled tools |
| --- | --- |
| None | Outline |
| Outline | Outline, Airfoil Stations |
| Outline and stations | Outline, Airfoil Stations, Airfoils |
| Outline, stations, and airfoils | Outline, Airfoil Stations, Airfoils, Dihedral |
| All above plus Dihedral | All seven tools, including Ailerons/Flaps, Spars, Lightening |

Earlier steps remain editable. Sequential progress selects the next incomplete
step. After Dihedral, all three remaining tools are enabled together; no optional
tool is selected automatically. An incomplete prerequisite disables all dependent
actions even if downstream definition flags remain set.

WingDefinitionState is owned by MainWindow. Future editors/model updates call
setWingDefinitionState after a definition is created or removed. Toolbar clicks
select tools only, never mark a definition complete. State survives workspace
switches; New resets it. Wing Outline now provides layered sketch editing (see
sketch-editor.md). Its connectivity check updates outline readiness while keeping
the current enabled tool selected during editing. Airfoil Stations now uses
curve-attached sketch lines (see airfoil-stations.md); two committed stations on every panel
unlock Airfoils. Airfoils supplies imported/traced profiles and station assignments
(see airfoils.md). Every station must have an assigned library airfoil to unlock
Dihedral. Its zero-degree defaults are already defined, so the final three tools
also become available at this point. Dihedral offers one Root Dihedral entry per
panel; see dihedral.md and wing-solids.md for geometry and regeneration.


Outline completion checks every numbered panel, regardless of the selected tab.
Panels 1 through N-1 require two independent open, unbranched segment chains
(LE and TE), with no tip connection. Panel N requires one open, unbranched chain
joining LE and TE through the tip. Either chain may mix lines and fitted splines.
Airfoil Stations is available only when all panels pass. Deleting an edge or
changing the panel count recomputes the entire wing's completion state.

The rules and action selection are applied by WingWorkflow.cpp. No persistent
format change is introduced. Solid construction is documented in ADR-0005.

Validation: wing_workflow_tests covers the initial state, each unlock step,
simultaneous unlocking of the final three tools, exclusive selection, stale
downstream flags after prerequisite removal, and toolbar reconstruction.

Windows Debug validation: configure/build passed; wing_workflow_tests,
reference_tests, and designrc_gui_tests passed (3/3, 0.84 seconds total).
The rebuilt build/debug/Debug/foamairplanestudio.exe launched successfully.
Linux/macOS were not tested. No changes were made in DesignRC.

Completing Wing/Airfoils also enables Fuselage in the primary toolbar, provided
Reference remains ready. Removing an outline/station/assignment prerequisite
locks Fuselage again; an active Fuselage workspace falls back to Wing (or Reference
when Reference itself is incomplete). Eligibility is recomputed on Open and New.
Toolbar navigation by itself neither changes document data nor regenerates an
unchanged wing model (ADR-0008).

Station completion and airfoil assignment are checked across all numbered panels (ADR-0013).
