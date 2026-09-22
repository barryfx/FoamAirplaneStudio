# ADR-0042: Circular profile sketches
Status: Accepted
Date: 2026-09-21

## Context
Edit Profiles needs a two-click circle tool with persistent editable geometry.
The user clarified that Formers receives no drawing tools.

## Decision
Add Circle (enum value 3) to the shared sketch editor, exposed only in Fuselage
Edit Profiles. Store two independent handles: center and radius point. Moving
the center translates both; moving the radius handle changes the radius.
The existing profile boundary pipeline samples circles at 256 angular intervals.
As with other profiles, generation fits width and height independently.

Format 28 accepts Circle only in fuselage profiles and their active tool. Formats
1–27 remain readable. Earlier applications reject format 28 explicitly. Escape
or switching tools discards an unfinished center-only circle.

## Alternatives Considered
Converting circles to ordinary splines loses their center/radius identity and
permits accidental non-circular edits. A constraint system is unnecessary.

## Consequences
No changes to former placement or other drawing toolbars. The existing profile
loft may produce elliptical cross-sections when width and height differ. The
sampled profile pipeline does not promise analytic-circle manufacturing output.

## Validation
Focused GUI, boundary and persistence tests without aircraft generation.
