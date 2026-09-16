# Width fitting and multipage PDF update — 2026-09-13

The Debug build in build/reference-validation-debug passed, including compilation
of updated GUI viewport and Reference tests. Automated tests were not run under
the user's existing instruction.

Coverage added for two PDF pages, aggregate physical height, stacked image items,
width fitting before/after resize, and vertical scrollbar availability.
No new interactive smoke check was performed for this revision. The previous
running app was left untouched to preserve the user's loaded reference/session.
Launch build/reference-validation-debug/Debug/foamairplanestudio.exe to use the
new behavior; the already-running older executable does not update in place.

No work was performed in DesignRC. Qt PDF remains the existing PDF dependency.

Follow-up: the mouse wheel now zooms; scrolling requires scrollbar interaction.
Horizontal scrolling is available when zooming beyond the viewport width. Actual
view resizing refits width, but scrollbar appearance does not cancel user zoom.
The follow-up Debug build passed; automated tests remain unrun.

Subsequent user-authorized focused tests passed (2/2); see reference-test-results.md.
