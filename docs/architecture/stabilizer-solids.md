# Stabilizer airfoils and solids

Both components use StabilizerAirfoilPanel. Its instructions, Load Airfoil .dat File
button and airfoil-name label appear in that order. The existing DAT loader handles
normalization, named/unnamed inputs and Selig/Lednicer layouts. Invalid loads and
cancelled dialogs preserve the selection. The shared file dialog uses the
stabilizerAirfoilDat history key. Each component independently defaults to the exact
resources/naca009.dat bundled in Qt resources, loaded before any tab visit. There
are no station assignments. Version 17 embeds custom profiles; earlier versions
default to NACA009. See formats/foam-project.md.

Only Outline is enabled until there is one open connected chain, endpoint alignment
within 10 degrees of either drawing axis, a selected leading-edge root endpoint, and nonzero area after temporary root
closure. Deleting necessary geometry locks dependent tools again. This is a drawing
readiness check; the solid builder additionally checks usable sections and topology.

StabilizerSolidBuilder consumes an immutable outline, one normalized airfoil,
millimeters-per-scene-unit scale and horizontal/vertical choice. It does not call
Wing or Fuselage generation. The two endpoints define the root chord. The user explicitly selects the leading endpoint using the Outline button.
No direction is inferred from horizontal/vertical placement. The span points toward the outline. Reversing curve traversal
does not reverse the airfoil. Outlines must lie on one side of the root line and
have unambiguous chord sections. Folded, pinched or disconnected sections fail
with an explanation instead of publishing an invalid result.

Actual-scale references use scene millimeters. Manual references use entered
Fuselage Length divided by the traced Side View longitudinal extent. This assumes
the stabilizer and Side View drawing share scale; it does not infer dimensions from
the entire image or from the stabilizer's bounding box. Independent per-component
scale overrides remain outside the current UI.

The builder samples the fitted closed planform and intersects it at 33 cosine-spaced
span positions plus outline control-point span positions. Each section uses the
selected airfoil, scaled proportionally to local chord, with upper/lower interpolated
curves and a tiny trailing-edge closure. OCCT makes a ruled solid loft. A narrowing
point tip closes at a vertex; finite-chord tips compress toward mid-thickness with
a circular retention factor near the outer end and a tiny finite closure. This is
a sampled approximation; no exact analytic skin or machining tolerance is claimed.

Local X is chordwise, Y is span, Z is thickness. Horizontal generation mirrors the
right half across Y=0 and fuses each matching pair into one solid body. With a hinge cut, the fixed stabilizer and elevator remain separate: two bodies total. Each pair must meet at the centerline; disconnected halves report an error. Vertical generation rotates its
single body so span is +Z and thickness is along Y. Model origin is at the root
leading endpoint; Assembly applies saved X/Z translations (assembly.md). Hinge Line separates the elevator/rudder before final orientation.

Each component owns its BackgroundJob, fingerprint, shape cache and camera state.
Entering 3D generates without requiring an Airfoil visit. Outline/airfoil/physical
scale changes invalidate the relevant cache; toolbar navigation and saving do not.
Workers only access captured data, local OCCT shapes and a copied status queue.
Stages report outline sampling, sections, lofting, validation, orientation/mirroring
and meshing. The existing shared processing lock and Cancel button apply to all
component jobs. Cancellation is cooperative and never publishes a cancelled result;
the prior display remains, and re-entering 3D retries. Kernel validation/transforms
may delay cancellation until their safe checkpoint. Project epochs and input
fingerprints reject stale results. Closing cancels and joins before the save prompt.
Successful replacement preserves the camera; first display fits automatically.

See ADR-0028 and baseline/stabilizer-model-validation.md for validation.

Hinge Line drawing and relief are described in stabilizer-hinges.md.

The shared Cancel button appears at the bottom of the data panel only during regeneration, outside the disabled editing area. Cancellation also covers centerline joining.

Closed Cut Shapes remove material after joining and may increase body count; see stabilizer-cuts.md.

Cache fingerprints are committed only after successful model publication. A failed
or cancelled attempt does not overwrite the previous successful shape/fingerprint.
Entering 3D with matching model inputs restores that component's cached shape,
camera and readiness/body count without starting a worker. Returning from another
workspace and moving between 2D/3D are covered by worker-start-count assertions.

StabilizerBuildResult exposes fixed and moving roles separately through Cut Shapes,
joining and orientation for Assembly collision checks. The shape-only builder
remains available to existing callers.
