# Per-panel wing dihedral

Wing / Dihedral replaces Wing Tip. Numbered tabs match the wing panels. Each has
one Root Dihedral spin box, in degrees, initially 0. Values range from -80 to 80,
with three decimal places. Positive values raise the panel; negative values lower
it. The first value is relative to horizontal and later values are increments
relative to the previous panel. Generation rejects cumulative angles at or beyond
85 degrees in either direction to avoid singular/reversed span construction.

Adding a panel creates a zero value, removing it discards its value, and changing
tabs does not dirty geometry or the project. Edits regenerate when Wing/3D is
visible or on next entry, retaining the camera. All project lifecycle operations
preserve the per-panel values and selected tab. Old versions 1-6 initialize all
angles to zero and map a saved Wing Tip toolbar selection to Dihedral.

Geometry uses the common root-derived chord/span frame. Nonzero angles construct
mitered end faces before rotating each complete panel (including spars, split
halves and controls) around its chord axis. The first root uses tan(angle) span
shear per unit height so the mirrored caps meet on Y=0. Internal joint end faces
use opposite tan(increment/2) shears and lie on the same angle-bisector plane.
Intermediate sections interpolate that span shear. Each new panel starts at the
preceding panel's generated tip and adds its root angle to the accumulated angle.

The span used to raise/place panels is their generated unfolded length, including
any near-terminal internal cutoff. Manual wingspan calibration continues to use
the full traced unfolded span; raising panels therefore reduces projected span.
The zero-angle path retains the flat trace placement of older projects.

Mitered caps project their chord onto an equal-span plane. Adjacent panels retain
their independently assigned profiles and chord dimensions: matching profiles and
chords have matching whole faces; different definitions can have a step at the
shared joint plane. The app does not silently replace airfoil assignments.

Spar tools sample inside both mitered end caps and overrun the caps where the spar
reaches a panel end. The solid boundaries trim the tools. Mid splitting and control
separation still occur per panel before assembly; dihedral moves every resulting
body together. The outermost tip always rounds toward the local upper/lower skin
midpoint. See ADR-0016 and wing-solids.md.
