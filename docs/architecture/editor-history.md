# Editor selection and undo/redo

With drawing tools off, clicking an individual line, spline or circle selects it
with the shared orange/white highlight. Delete removes just that curve and
compacts point indices while updating dependent station anchors. Stabilizer Cut
follows the same rule; its explicit Delete Cut Shape button removes a whole loop.

Edit > Undo (Ctrl+Z) and Edit > Redo (Ctrl+Y) share one chronological project
history across sketch editors and data panels, including stations, profiles,
control surfaces, formers, servo trays, Weight and Balance parts and Assembly
placements. Held-button drags and click-to-drop station moves count as one edit.
Unfinished drawing points can also be undone/redone. Selection, camera movement
and workspace navigation do not add revisions.

The latest 100 revisions retain typed ProjectDocument inputs and implicitly
shared reference images, never generated OCCT geometry. Restoring a revision
preserves the project filename and saved-state fingerprint. Generated caches
continue using their input fingerprints; entering a model view can regenerate
stale geometry. Undo does not generate components. Save retains history; New,
Open and Close clear it. New edits after Undo discard the redo branch. History
is not saved to .foam files. See ADR-0043.
