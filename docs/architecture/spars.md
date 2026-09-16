# Wing spars

Wing / Spars shows numbered tabs matching the outline panels. Each panel has
independent Top, Bottom and Mid wing-height options, initially unchecked.
Chord percentage is measured from the local LE; length is a percentage of that
panel's span starting at its root. Size entries accept mm or in regardless of project units and are stored
in millimetres. Entered numeric text and units remain visible across unit changes,
panel switches and Save/Open. Bare entries use the current project units and gain
an explicit suffix when committed. Untouched defaults display in project units.
Invalid or out-of-range edits restore the previous committed value when focus
leaves the field or Enter is pressed. Top/Bottom offer Round (diameter) or Strip (width and inward
height/depth); Mid is round only. Defaults are 30% chord, 80% panel length,
3 mm diameter/width and 1 mm strip depth. Unchecking retains values.

New panels have disabled spars. Removing a panel removes its settings. Selecting
a tab changes only view state; values in other tabs are retained. All lifecycle
operations preserve spar data, and Save preserves the current spar tab. Unit
changes retain physical dimensions. Data edits regenerate in 3D or when next
entering it, preserving the camera. Navigation alone does not regenerate.

Only the main wing is partitioned by panel and, when Mid is enabled, by height.
Mid sits at (upper + lower skin)/2 at its chosen chord percentage. Its split
continues chordwise through that center, follows the height along the spar, and
continues at its terminal height beyond the spar end to the panel tip;
it no longer uses Z=0. The hole stops at the entered length. Alignment tabs and
holes at 20%/80% panel span follow the moved split. Ailerons/flaps bypass both
panel and height splitting and remain separate intact bodies before mirroring.

Errors identify panel and Top/Bottom/Mid. Thickness failures report location and
required/available material. Thin tips and intersecting grooves can still be
invalid; Mid must fit within local total thickness. See ADR-0012 for sampling,
span boundaries, tab fit and migration. Version-3 global spar settings load into
Panel 1; other panels start disabled and should be configured explicitly.

A 100%-length internal-panel spar ends at the generated panel end, including the
near-terminal-station cutoff described in wing-solids.md. At an oblique root,
its first skin sample is taken just beyond the full cap and the tool extends
back through the cap, avoiding false near-zero corner thickness readings.

With dihedral, sample skin inside both mitered joint caps and extend root/full-length
cutting tools through those caps. After cutting and any mid split, all panel bodies
move together into the accumulated dihedral frame; see dihedral.md.

Lightening also requires splitting the main panel when Mid is disabled, using
half thickness at 30% chord. Existing Mid splits and alignment features remain;
hollowing preserves wall-thickness support around pins and sockets. See lightening.md.
