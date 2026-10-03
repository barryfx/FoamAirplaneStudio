# Stabilizer Cut Shapes

Each stabilizer Cut panel starts with instructions, Add Cut Shape, a numbered
shape selector, Line/Spline toggles, and Delete Cut Shape. Each shape is an
independent SketchEditor layer. Add selects a new empty layer (reusing the initial
empty layer). Lines join at shared endpoints; a spline closes by clicking its first
point. Escape finishes a spline. With drawing off, click a loop or choose it from
the list and drag its points. Click a curve to highlight it and press Delete to
remove that individual curve. Delete Cut Shape removes the entire selected loop. Other shapes remain
visible. Empty shapes do not cut. Only the active component in Cut/2D accepts input.
Both stabilizers' cut shapes use yellow in every 2D view, including during
selection; see [Sketch editing](sketch-editor.md) for the shared viewport palette.

Leaving Cut, including entering 3D, warns if any nonempty loop is not closed.
Incomplete curves remain editable; generation refuses them rather than publishing
an uncut model. All loops are validated before a component, Assembly or Inspect
worker starts, and again at the builder entry point before lofting or hinge work.
A Cut Shape must be one connected, unbranched, nonzero-area loop.
New/Close resets shapes. Reference scaling remaps every layer. Version 20 saves
layers, selected shape, drawing tools and drafts; older files initialize empty cuts.

Cut Shapes remove material inside their loops, through the full model thickness.
The builder interpolates spline edges, creates planar faces and extruded Boolean
tools, and subtracts them from every intersecting body. No kerf is added. All valid
remaining solids are retained, including pieces separated by a through-slot.
Nonintersecting tools leave the model unchanged; removing the entire model errors.

Cuts run after horizontal centerline joining, so they may separate previously
joined bodies. Tools drawn on the right half are also mirrored; overlapping tools
may cut the centerline. Vertical cuts run before the fin is rotated upright. Both
fixed and control bodies are eligible. Closed-loop topology is checked first;
OCCT validates faces and resulting solids. Invalid loops/Booleans report an error.

Cut layers participate in each stabilizer's background-job fingerprint. Selection
and idle drawing-tool changes do not dirty the project; geometry and pending points
do. Cancellation is cooperative through modeling checkpoints. The shared bottom
Cancel button is visible only during generation, for every component.
