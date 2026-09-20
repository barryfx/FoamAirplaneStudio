# Assembly component export

Export is enabled only after a current Assembly has successfully generated.
Component caches or complete definitions alone do not enable it. Input changes,
New, Close and Open invalidate availability. Export uses `exportAssemblyParts()`:
placed originals before Cut Intersections, or the finished cut snapshot afterward.
It does not regenerate components. Export shows Assembly in 3D with 2D disabled.

The data panel contains instructions, independent exclusive Formers STEP/DXF/STL and
Components STEP/STL groups, All, the part checklist, and Export Components at the
bottom. The list scrolls independently of the button. Initially nothing is checked;
All checks or clears every entry and reflects whether all entries are selected.
No selection disables the export button. Formers appear first, numbered from 1
by source mask center from nose to tail, irrespective of insertion order. Other
entries identify each fuselage/wing/stabilizer/control solid and the servo tray.
Multiple solids from one named component receive numeric suffixes.

Assembly keeps formers and the servo tray as standalone inserts, separate from
the fuselage body. Seat cuts affect only body pieces; inserts remain unchanged
and are exported independently, never joined to the fuselage.
Each former retains its rotated local mid-plane alongside its finished geometry.
This metadata is session-only and adds no fields to the project format. Opening a
project saved in Export returns to Fuselage Side View in 2D without regeneration.

Export Components uses FileSelectionDialog with `componentExportDirectory`, so
the accepted folder persists across exports and application restarts. Cancel does
not change it. Existing output names prompt before replacement. CAD conversion
finishes in a temporary directory before destination files are replaced atomically
with QSaveFile. A write failure reports how many files were already committed;
the entire multi-file batch is not a single filesystem transaction.

STEP is the default for both format groups. Selected formers and other components
whose group uses STEP share one `Components.step`. With Components STL selected,
Formers STEP still writes the selected formers to that file while other components
produce individual STL files. Formers remain separate named solids in STEP.
The inherited DesignRC STEPCAF exporter writes AP242 with named, separate BREP
parts under a Components assembly. Its original Wing hierarchy remains the default
for legacy callers. No material colors or extra mirroring are introduced.
STL produces one binary `<part name>.stl` per selection, retaining Assembly placement.
STL coordinates are millimeters. Meshing uses a private shape copy with 0.05 mm
linear and 0.15 rad angular deflection, preserving cached/displayed meshes.

Former DXF produces `<Former N>.dxf` at 1:1 millimeters. It sections the actual
Assembly former at its local thickness midpoint, flattens that plane, and retains
all closed outer/inner contours. Curves are sampled into closed polylines with
0.02 mm deflection. Rotation does not shorten the drawn section. This represents
the mid-plane profile, not a bevel allowance or maximum silhouette through varying
thickness. Invalid/open/empty sections report an error. Former STL uses the full
finished solid. Servo Tray is a component exported as STEP/STL; SVG and tray DXF
remain future work. Formats and checklists are transient UI choices.

See ADR-0037 and `../baseline/export-validation.md`.
