# ADR-0008: Separate project data, view state and generated model inputs
Status: Accepted
Date: 2026-09-13

## Context
Sketch refresh signals also occur when changing modes or selections. Comparing
all serialized project fields marked camera/navigation changes as unsaved work,
and unconditional invalidation regenerated unchanged geometry.

## Decision
Continue saving the complete version-1 project, including UI state. Compare a
separate data-only representation for save prompts: reference data and entered
text, outlines, pending sketch points and their meaning, stations/assignments,
airfoil entries and drafts, and tip selection. Exclude workspace/tool navigation,
viewport/camera/scroll/splitter state, and idle editor/library selections.
New and Open establish this data baseline immediately; layout events cannot
silently mark a subsequent user edit as saved.

Compare committed wing inputs before generation: outline layers, station lines
and assignments, library entries, effective manual scale, tip choice and readiness.
Ignore broad refresh signals when those inputs equal the previous attempt.
Incomplete/failed attempts are also remembered until their inputs change.
Reset/Open clears this cache. Pending sketch points become model inputs when
committed. Switching modes can still commit an unfinished sketch or assign an
unassigned station; those are actual data changes.

On the first model display, fit the new wing (or restore the saved camera).
For replacements retain eye, center, up, scale, field of view and projection,
including after a temporarily invalid model. Refresh depth clipping for the new
bounds without fitting the camera. Explicit view/fit commands remain available.

Fuselage becomes available when Reference is ready and the Wing outline, stations
and all airfoil assignments are complete. Removing prerequisites disables it again;
if active, return to Wing or Reference as appropriate. This enables the existing
Fuselage workspace; its geometry/data editors remain future work.

## Alternatives Considered
Changing every reusable sketch signal would still leave duplicate notifications
and UI/data coupling elsewhere. A single dirty flag cannot distinguish selection
changes from actual input changes or recognize returning to the saved data.
Discarding UI persistence would lose requested project restoration behavior.

## Consequences
Mode/view choices persist on an explicit Save but do not prompt saving on their
own. No file version change or geometry algorithm change is needed. Model input
comparisons use lightweight in-memory JSON rather than image encoding or OCCT.

## Validation
Project and station workflow regressions cover navigation without dirty state or
regeneration, open/new without save prompts, immediate edits after New, camera
retention through live/deferred updates, and Fuselage eligibility/restoration.
