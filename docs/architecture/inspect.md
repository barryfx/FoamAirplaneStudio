# Inspect workspace

Inspect follows Assembly in the primary toolbar, followed by Weight and Balance
and Export. It opens the normal 3D viewport with
2D disabled and a scrollable vertical list of checkboxes and editable names.
It uses placed/cut Assembly geometry when current, otherwise current individual
Wing, Fuselage and stabilizer caches. Each manufacturing solid is listed,
including separate formers and servo tray; stale model caches are excluded.
Entering Inspect reuses the existing Assembly preparation worker and component
fingerprints to regenerate changed or missing models with complete definitions.
Unchanged caches are reused; incomplete components are omitted. The normal
processing lock, busy cursor, progress and Cancel remain active. Cancellation or
failure publishes no partial worker result; re-enter Inspect to retry. An empty
list explains that complete definitions are needed. The reference background is hidden in Inspect.

Every new component starts visible. Unchecking removes it from the displayed
shapes immediately without changing camera orientation/scale. Visibility survives
workspace switches within the session, resets on New/Open, and is independent of
Export selection and Weight and Balance mass. Orbit, pan, zoom, View menu and
the viewport camera toolbar use their normal handlers. Other workspaces restore
their complete component views.

Name edits commit on Enter or focus loss. Empty, duplicate, reserved device names,
filename punctuation/control characters and trailing dots are rejected. Names
are limited to 120 characters. Invalid input reverts and displays an explanation.
Names, introduced in format 27, are saved by source component identifier and solid ordinal,
not by mutable display text. Unavailable component aliases are retained so normal
regeneration can reuse them. Topology-changing edits that reorder or replace
solids can change ordinal identity; inspect names after such edits (ADR-0041).

The Export checklist, STEP part labels and individual DXF/SVG/STL filenames use these
names. Combined STEP output uses the saved project basename, or Untitled for an
unsaved project. Project filename characters invalid on another platform are
sanitized. Export rejects duplicate component names/filenames before writing.
Aliases do not alter component generation or Assembly/mass fingerprints.

Opening a file saved in Inspect returns to the editing workspace without
regeneration, consistent with Assembly/Export. Names remain saved, but Inspect
regenerates the defined models on the next explicit entry.
