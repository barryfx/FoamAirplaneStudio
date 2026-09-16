# ADR-0006: Complete versioned project snapshots
Status: Accepted
Date: 2026-09-13

## Context
Users need New, Open, Close, Save and Save As for the drawing-based workflow,
including its reference, physical scale, semantic sketches and current editing
and viewport state. The historical DesignRC serializer was removed and does not
represent these models.

## Decision
Use a new `.foam` JSON document with `format: FoamAirplaneStudio` and integer
`version: 1`. ProjectDocument is a value snapshot; its codec is independent of
File-menu actions. Component editors expose explicit state capture/restoration.
See docs/formats/foam-project.md for fields and limits.

Embed rendered reference pages as lossless PNG/base64, along with the original
source filename and each page's physical dimensions. Opening uses the embedded
snapshot, so relocated or changed external images/PDFs cannot change the saved
project. Imported airfoil coordinates and traced library snapshots are likewise
self-contained. Original PDFs are not embedded as PDF documents; their current
rendered pages and scale metadata reproduce the existing viewport.

Persist all current semantic models, unfinished sketch points and named airfoil
drafts, active tools/panel/station/airfoil selections, workspace, viewport tab,
2D zoom/center, 3D camera and data/viewport splitter sizes. Mouse capture and hover
previews are transient. A moving point's current coordinates are saved without
resuming a held mouse gesture. An unfinished station's first anchor is retained.
Generated OCCT solids are derived and rebuilt when opening into Wing/3D.

QSaveFile performs atomic replacement; Save As changes the current filename only
after success. Parse and validate a complete candidate before prompting to replace
the current project. Reject unsupported versions, invalid indices, bad image data,
non-finite/out-of-range numbers and invalid camera orientation. No filesystem or
application actions are taken from project contents. Legacy `.designrc` files
are not silently treated as the new format.

New, Open, Close Project and window exit use Save/Discard/Cancel when modified.
Canceled dialogs or failed saves preserve the current project. Closing a project
leaves the app open with its editors disabled; New/Open reenable them. All file
dialogs share the existing remembered-selection utility.

A lightweight JSON fingerprint detects changes, including view/selection changes,
without repeatedly encoding image pixels. The image cache key is used only for
this in-process comparison; it is never a reference in a saved file. Restoration
suppresses intermediate regeneration and records a clean baseline after layout
and view events settle. No global project state or new dependency is added.

## Alternatives Considered
External-only reference links would break restoration when files move or change.
A ZIP package would add packaging machinery; PNG already compresses page pixels.
Serializing OCCT geometry would duplicate derived state and constrain future
modeling changes. Reusing `.designrc` would imply unsupported legacy compatibility.

## Consequences
Projects are portable and inspectable but embedded reference pages increase file
size. Format migrations must be explicit; unknown versions are rejected. Window
placement, global dialog history and OS mouse gestures are not project state.
The existing finite section sampling and model-generation limits still apply.

## Validation
ProjectTests covers embedded multi-page references, units/scale, sketches and
drafts, airfoil library/assignments, malformed input, Save As, unsaved-change
prompts, cancellation, failed writes, Close/New, 2D view restoration and 3D
regeneration with camera restoration. Existing GUI/workflow tests remain relevant.
