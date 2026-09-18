# ADR-0013: Panel stations, shared airfoils and panel controls
Status: Accepted
Date: 2026-09-14

## Context
Airfoil Stations, Airfoils and Ailerons/Flaps need the same numbered panels as
Outline and Spars. Each panel owns its stations, assignments and control cuts.
Imported and sketched airfoils must be selectable throughout the project.

## Decision
Keep one shared airfoil library and one station collection. A station belongs
to the outline layer referenced by both anchors. The active panel limits curve
snapping, station movement/deletion and station assignment. All committed
geometry remains visible. Require two stations on every panel and assignments
on every station before enabling downstream tools.

Numbered tabs use the outline panel count. Airfoils retains a per-panel current
library choice, while each station retains its own assignment. Airfoil tracing
creates shared library entries; tabs are disabled while tracing. Ailerons/Flaps
owns an independent two-control record per panel. Overlap validation compares
all enabled rectangles, including across panels. Only the selected panel's
rectangles can be edited. Removing a panel removes its stations and settings;
new panels start with no stations or control cuts. Library entries survive.

Generate each panel using only its own stations and controls in a common
root-derived coordinate frame and global scale. Separate panel lofts permit
different airfoils at a shared boundary without silently interpolating across
panels. Only the outermost panel receives wing-tip shaping. Control cuts affect
only their owner panel, then spars split only its fixed body. Assemble the panel
bodies and mirror the entire half-wing at the global root. Profiles at mating
panel faces are not automatically made identical; the user controls them.

Version 5 stores panel control arrays and selected panel/airfoil choices.
Versions 1-4 still load. Former global control settings migrate to Panel 1,
with other panels disabled; existing station anchors and shared library indices
are retained. Legacy multi-panel projects may need additional stations and a
review of control placement. No original file is changed until Save.

## Alternatives Considered
Duplicating libraries would prevent cross-panel airfoil reuse. A global loft
would blend profiles across panel boundaries or drop coincident stations with
different assignments. Automatically duplicating old controls into every panel
would introduce cuts that the user did not request.

## Consequences
Panel data is independent; library data stays shared. Panel/tab selection alone
is view state and does not regenerate geometry or dirty an assigned project.
Choosing an unassigned station retains the existing default-assignment behavior.
Internal panel joins become separate part boundaries. This supersedes the
whole-wing loft and cross-panel control-body portions of earlier ADRs while
retaining the established sketch, hinge and spar geometry rules.

## Validation
Editor tests exercise ownership, panel switching, shared library entries,
independent control settings and cross-panel overlap rejection. Persistence tests
cover version 5, legacy migration, panel choices and count validation. Solid tests
check independent profiles at a shared boundary, separate panel flaps, mirroring
and spar/control separation. Full workflow tests and Debug launch are required.
