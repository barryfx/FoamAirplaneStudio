# ADR-0039: Assembly-based weight and balance
Status: Accepted
Date: 2026-09-20

## Context
The new Weight and Balance workspace places named RC components on the fuselage
Side View and combines their weights with generated airframe material. Results
must use Assembly placement and cuts, and survive project save/open.

## Decision
Store component dimensions and center positions in physical millimeters in the
fuselage X/Z frame, with Y=0. A part has uniform mass centered in its rectangular
envelope. Width is transverse, height vertical, and length nose to tail. Store
mass in grams and retain each part's grams/ounces entry preference. Draw parts
only in this workspace; dragging and list selection refer to the same part.

Use current placed/cut Assembly solids for adaptive OCCT volume/centroid
integration. Count the fuselage body once, plus wing, fixed stabilizers and
controls. Separate former and servo-tray inserts use Aero Plywood density.
Default to 32.5 kg/m³ XPS and 680 kg/m³ birch Aero Plywood, both editable.
These are representative stock values, not universal material constants.

The datum is the wing root leading-edge endpoint projected into the same chord
frame and scale as WingSolidBuilder, plus the Assembly wing X translation. It
is not the most forward point of a swept tip or an inferred fuselage wing seat.
Positive distances run toward the tail; display them in Reference units.

Format 26 adds the parts and both densities; versions 1–25 load empty parts and
default densities. Geometry remains session-only. Weight and Balance never
starts component generation. Without current Assembly geometry it reports parts
weight and marks the full total and center of mass unavailable. Model edits must
be regenerated in Assembly before full results return.

## Alternatives Considered
Persisting scene-space rectangles couples physical size to image remapping.
Using bounding-box centers or mesh volumes loses solid mass accuracy. Including
insert volume as foam misrepresents plywood; adding it twice also overcounts mass.
Persisting calculated geometry would reverse the input-only persistence decision.

## Consequences
Part moves and density edits recalculate weighted moments without CAD generation.
Parts do not subtract material or create collision constraints. Spars, coverings,
adhesives and other unmodeled material must be entered as additional parts.
Mass properties are cached across workspace entries using generation fingerprints,
Assembly placement/cut state, project epoch and immutable source-solid identities.
Only a cache miss measures solids, with the standard status message and busy cursor.
Opening retains inputs and shows unavailable full results until Assembly exists.

Density references: [DuPont XPS billet](https://www.dupont.com/content/dam/dupont/amer/us/en/performance-building-solutions/public/documents/en/styrofoam-pipe-insulation-billet-43-d101096-enna.pdf)
and [Broszeit birch aircraft plywood](https://broszeit-group.com/produkt/birch-aircraft-plywood/?lang=en)
(published range 680–750 kg/m³; editable starting value uses its lower end).

## Validation
The dedicated Weight and Balance checks use analytical boxes and editor inputs,
without Wing/Fuselage generation. See baseline/weight-balance-validation.md for
executed results and visual checks.
