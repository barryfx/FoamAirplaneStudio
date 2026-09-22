# ADR-0043: Project editor undo history
Status: Accepted
Date: 2026-09-22

## Context
Edits span sketch curves, stations referencing those curves, assigned profiles,
placements and data panels. Undoing only one controller can leave dependent data
inconsistent. Generated OCCT solids are cached independently of design inputs.

## Decision
Keep up to 100 before/after ProjectDocument revisions in MainWindow. Compare the
existing navigation-independent project fingerprint. Qt input boundaries capture
edits; held-button drags and click-to-drop station moves are captured only when
finished. A periodic capture covers panel/controller changes outside input events.
Restore through the existing project restoration path, preserving the filename,
save-point fingerprint and generated cache fingerprints. Undo itself does not
start component generation. New/Open clear history; Save retains history.
Images use QImage shared storage; generated solids are not copied into revisions.

## Alternatives Considered
Per-editor command stacks would duplicate dependency restoration and split the
user's chronological history. Serialized snapshots would duplicate image bytes.

## Consequences
All project editors share Ctrl+Z/Ctrl+Y and Edit menu actions. Navigation and
selection do not create revisions. Pending drawing points are recoverable edits.
History is transient and bounded, with no project format change. Memory usage
scales with design-input size rather than generated geometry size.

## Validation
Dedicated editor_history_tests checks GUI shortcuts, selection/deletion, drag
grouping, all sketch collections, non-sketch editor restoration, save points,
redo branch replacement and history reset without component generation.
