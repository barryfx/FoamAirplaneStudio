# FoamAirplaneStudio Software Requirements

## 1. Purpose

FoamAirplaneStudio is a desktop software application for designing foam model airplanes intended to be fabricated with a CNC router. The application shall use Open CASCADE Technology (OCCT) for geometric modeling and solid-body operations.

The primary workflow is:

1. Wing design
2. Fuselage design
3. Horizontal stabilizer design
4. Vertical stabilizer design
5. Airframe component placement and interface generation
6. Final 3D assembly review
7. Export of CNC/CAD-ready geometry

The software shall support designing an aircraft from imported reference drawings that may or may not be accurately scaled.

## 2. General Requirements

### 2.0 Project Lifecycle

- New, Open, Close Project, Save and Save As shall manage complete `.foam` projects.
- Saved state shall include source image filenames and embedded reference pages,
  units and scale, sketches and their roles, stations, airfoil data/assignments,
  dihedral, control/spar/lightening settings, unfinished sketches, active modes and 2D/3D viewport state.
- Opening shall restore a project without requiring its external source files.
- Unsaved changes shall offer Save, Discard or Cancel before replacement or exit.
- Invalid project files and failed/canceled saves shall preserve current work.
- Project writes shall replace files atomically and use a versioned format.

### 2.1 Reference Drawings

- The software shall allow the user to import airplane reference images/drawings for tracing geometry.
- The user shall be able to trace component outlines and profile shapes over the imported drawings.
- When a drawing is not to scale, the software shall allow the user to enter a known physical dimension and scale the derived geometry accordingly.

### 2.1.1 Reference Controls and Project Units

- Reference mode shall show Load Image and the loaded file path.
- Load Image shall support PNG, JPG/JPEG, and PDF backgrounds in the 2D view.
- The user shall choose between Reference is to scale and Specify Dimensions.
- Actual-scale mode shall derive physical image/page dimensions and project units
  from the source metadata. Missing physical metadata requires manual dimensions.
- Specify Dimensions shall show a Project Units selector for inches/millimeters
  and decimal Wingspan and Fuselage Length fields, arranged vertically.
- These units and dimensions shall apply to the entire project.

### 2.2 Geometry and Modeling

- OCCT shall be used for construction of curves, surfaces, solids, shells, Boolean operations, splitting, interpolation/lofting, and export geometry where applicable.
- The software shall maintain separate solid bodies for airplane components that will be manufactured separately.
- The application shall provide a 3D viewport for viewing geometry throughout the design process.

## 3. Wing Design

### 3.1 Wing Planform

- Wing design shall begin by importing a reference drawing and outlining the planform of each wing panel.
- Individual panel outlines shall combine to define one half of the complete wing.
- The opposite half of the wing shall be assumed to be a mirror image.
- The user shall be able to enter the actual wingspan when the reference drawing is not to scale.
- The entered wingspan shall be used to scale the traced wing geometry.

### 3.2 Airfoil Profile Stations

- The user shall select locations along the wing at which airfoil profiles are defined.
- An airfoil profile at a station may be obtained by:
  - Importing airfoil coordinate data from a `.dat` file, or
  - Drawing/tracing an airfoil outline over a reference image.
- The local wing chord at each profile station shall be determined from the distance between the wing planform boundaries at that station.
- The imported or traced airfoil shall be scaled to the local chord.
- Airfoil scaling shall occur in both the X and Y dimensions so that the complete airfoil geometry scales proportionally with chord.

### 3.3 Wing Solid Generation

- The software shall interpolate/loft between the defined airfoil profiles to construct the wing geometry.
- The generated geometry shall form solid wing bodies suitable for subsequent OCCT Boolean and splitting operations.
- Wing geometry shall follow the traced panel planforms and profile stations.

### 3.4 Dihedral and Wing Tips

- Dihedral shall replace the Wing Tip toolbar tool and tip-choice buttons.
- Each wing panel shall have its own Root Dihedral field in degrees.
- The first angle is relative to horizontal; each subsequent angle is relative to the preceding panel.
- Geometry shall raise each panel tip according to its accumulated angle and place each outer root at the preceding panel tip.
- Root and panel-joint faces shall be angled to mate, including the mirrored half-wing centerline.
- The outermost wing tip shall always use a rounded profile looking along the chord and retain the traced outer tip contour.
- The user shall no longer select a wing-tip type.
- Project files shall preserve per-panel angles and the selected Dihedral panel. Older projects shall load with zero degrees for every panel.
- Assigned airfoils shall automatically generate a mirrored solid wing in Wing/3D mode; changed wing parameters shall regenerate it there or on the next entry to 3D.

### 3.5 Ailerons and Flaps

- The user shall be able to define separate aileron and flap parts.
- Aileron and flap regions shall be defined using rectangular selection geometry.
- The intersection between the selection rectangle and the wing solid shall determine the control-surface cut boundaries.
- The resulting aileron and flap geometry shall be separable into independent solid bodies.

- Ailerons/Flaps mode shall provide independent Add Ailerons and Add Flaps checkboxes, initially unchecked, with conditional rectangle instructions.
- Each enabled control shall offer Tape Hinge Cut (default) and Standard Hinge Cut as mutually exclusive choices.
- Tape hinge relief shall cut the control side at 45 degrees, retaining top contact and opening a bottom gap.
- Standard hinge relief shall cut the control side at 45 degrees above and below center-thickness contact.
- Rectangle drawing shall support opposite corners and Escape to exit; outlines shall remain locked and enabled rectangles shall have distinct colors.
- All control settings and rectangle meanings shall be saved/restored with the project.
- Both spanwise ends shall have 1/16-inch clearance removed from the control surface.
- Outside drawing mode, clicking a rectangle shall highlight it; Escape shall clear selection and Delete shall remove it.
- Leaving the mode or selecting 3D shall uncheck enabled controls without a defined rectangle.

### 3.6 Internal Lightening

- The software shall provide an option to create internal lightening regions within the wing solid.
- Lightening operations shall remove internal foam while retaining the required exterior and structural geometry.

### 3.7 Structural Slots and Spar Features

- The user shall be able to add slots or openings for structural stiffening members.
- Supported stiffening features shall include members such as:
  - Spars
  - Stiffening strips
  - Stiffening rods
- These slots/openings are structural features and are distinct from the internal lightening regions.
- The software shall allow the user to create a wing-spar hole through the wing.

- Numbered spar tabs shall match outline panels. Each panel shall have independently enabled Top, Bottom and Mid options.
- Chord percentage shall be measured from the local leading edge; length percentage shall be measured from that panel root relative to its span.
- Top/Bottom spars shall offer round diameter or strip width and height in project units. Mid shall use a round hole centered at 50% local thickness at the selected chord location.
- A Mid split shall include tabs and matching holes on both sides of the spar, about 3 mm away, at 20% and 80% of the owning panel span.
- Project files shall retain all spar settings; model regeneration shall apply them.

### 3.8 Wing Manufacturing Split

- Enabling a Mid spar shall split the fixed wing into upper and lower bodies along a split surface through the Mid spar center. Ailerons/flaps shall remain intact. Top/Bottom spars alone shall not require this split. Lightening shall require an access split even without a Mid spar.
- The resulting bodies shall be suitable for separate CNC routing operations and subsequent assembly.

## 4. Fuselage Design

### 4.1 Fuselage Reference Geometry

- Fuselage definition shall begin by tracing the fuselage outline from the 2D view/Referrence Image.
- If the reference drawing is not to scale, the user shall enter the actual fuselage length.
- The software shall scale the traced geometry based on the entered fuselage length.

### 4.2 Fuselage Profile Stations

- The user shall select cross-section/profile locations along the reference image.
- Cross-section planes shall be perpendicular to the fuselage longitudinal direction.
- Profile stations shall be numbered sequentially.
- Profile 1 shall be located at the nose of the fuselage.

### 4.3 (SECTION DELETED)

### 4.4 Profile Editing

- The user shall be able to sketch the profile over the corresponding portion of the reference drawing.
- The profile outline shall contain editable lines and fit point splines.
- The user shall be able to move control points to refine the fuselage cross-section.
- Changes to profiles shall update the estimated 3D fuselage geometry.

### 4.5 Fuselage Solid and Shell

- The software shall generate the outer fuselage geometry from the top outline, side outline, and cross-section profiles.
- Once the exterior shape has been determined, the user shall be able to convert the fuselage into a shell.
- Fuselage wall thickness shall be configurable at each profile station.
- The software shall provide initial/default wall-thickness values based on fuselage length.
- The user shall be able to modify these thickness values.
- The nose of the fuselage will be open.  The tail will be closed.

### 4.6 Servo Tray

- The user shall be able to specify a servo-tray location within the fuselage.
- The user shall provide the required servo-tray dimensions.
- The software shall create ledges on the inside surfaces of the fuselage sides to support the servo tray.
- The ledges shall provide surfaces suitable for positioning and gluing the servo tray into place.
- The software shall generate a solid body model for the servo tray exportable in STEP, STL, DXF, or SVG
  formats.

### 4.7 Firewall

- The user shall be able to designate the location and thickness of a firewall.
- The software shall derive the required firewall solid body from the fuselage inside shape at the
  selected location.
- Slots of the requested thickness shall be generated in both halves of the fuselage raised from the
  inside surface to locate and retain the firewall.
- The firewall may be exported in the STEP, STL, DXF, or SVG formats.

### 4.8 User-Defined Fuselage Part Separation

- The user shall be able to specify a separation line over the top or side fuselage reference view.
- These lines shall be used to cut the fuselage into two separate solid bodies.

### 4.9 Fuselage Manufacturing Split

- The completed fuselage shall be split horizontally into separate top and bottom bodies.
- This horizontal manufacturing split shall be independent of any additional user-defined part-separation cuts.

### 4.10 User-Defined Fuselage Add/Cut

- The may specify closed loops made from lines and/or splines for regions that may be cut from the fuselage.
- The may specify closed loops made from lines and/or splines for regions that may be added to the inside of the
  fuselage.  The width of the region shall be specified by the user.

## 5. Horizontal and Vertical Stabilizer Design

### 5.1 Common Stabilizer Workflow

- Horizontal and vertical stabilizers shall use the same general profile-based modeling workflow.
- Each stabilizer shall initially be modeled as a continuous body before its movable control surface is separated.

### 5.2 Symmetry

- The horizontal stabilizer shall be modeled from one half of the stabilizer and mirrored across the aircraft centerline to create the complete horizontal stabilizer.
- The vertical stabilizer shall not use this mirror operation.

### 5.3 Stabilizer Planform

- The stabilizer planform shall be traced over the appropriate reference drawing, including the side-view drawing where applicable.
- The planform shall initially be outlined as one continuous piece.

### 5.4 Stabilizer Profiles

- The user shall select profile locations along the fuselage-length direction of the stabilizer geometry.
- Profile shapes may be obtained from:
  - Imported `.dat` files, or
  - Profiles traced/drawn over the reference plans.
- The software shall interpolate between the profiles using the same general profile-lofting approach used for the wing.

### 5.5 Stabilizer Tips

- Stabilizer tip geometry shall be generated automatically by the software.
- The tip shall form a smooth transition from the final defined profile to the end of the stabilizer planform.
- The user shall not be required to create or select a separate stabilizer-tip shape.

### 5.6 Elevator and Rudder Separation

- The user shall specify the split line between the fixed stabilizer structure and the movable control surface.
- For the horizontal stabilizer, the split shall create separate stabilizer and elevator bodies.
- For the vertical stabilizer, the split shall create separate fin and rudder bodies.

### 5.7 User Defined Cuts

- The user may specify closed loops over the outlines to cut from the solid bodies.
- Cuts loops may be defined in lines and/or fitted splines.

## 6. Airframe Assembly and Component Interfaces

### 6.1 Wing Attachment

- After component modeling, the user shall select the wing attachment location over the fuselage side view.
- The software shall position the wing relative to the fuselage using the selected attachment location.
- Depending on the selected wing position, the software shall automatically determine whether the fuselage requires:
  - A conforming wing saddle, or
  - A wing-shaped opening/hole where the wing intersects or passes through the fuselage.
- The required mating geometry shall be cut into the appropriate fuselage bodies.

### 6.2 Horizontal Stabilizer Attachment

- The user shall select the horizontal stabilizer attachment location.
- The horizontal stabilizer may be positioned:
  - On top of the fuselage, or
  - Partway down/through the fuselage.
- The software shall generate a conforming saddle or stabilizer-shaped opening as dictated by the selected position.
- The mating geometry shall be cut into the appropriate fuselage body or bodies.

### 6.3 Vertical Stabilizer Attachment

- The user shall select the vertical stabilizer location on top of the fuselage and/or horizontal stabilizer.
- The software shall generate a fin slot from the upper surface of the supporting component.
- If the horizontal stabilizer is mounted on top of the fuselage and supports/intersects the fin, the slot may pass through the horizontal stabilizer as required by the selected geometry.
- The fin-slot depth shall not exceed the fuselage width at that location.
- If the fin penetrates into the fuselage, the software shall remove/trim the required portion of the fin body so that it fits the generated slot and mating geometry.

## 7. Final 3D Assembly

- After all components have been modeled and their attachment interfaces generated, the software shall display the fully assembled 3D airplane in the viewport.
- The assembled model shall show the relative positions of at least:
  - Wing
  - Fuselage
  - Horizontal stabilizer and elevator
  - Vertical stabilizer and rudder
  - Ailerons and flaps when defined
- The user shall be able to visually inspect the completed airplane before export.

## 8. Export

### 8.1 Solid Body Export

- The software shall allow generated solid bodies to be exported in:
  - STEP (`.step`)
  - STL (`.stl`)
- Export shall support the separate bodies created during the design process, including manufacturing splits and control surfaces.

### 8.2 Firewall and Servo Tray Export

- The firewall and servo tray outlines/profiles shall be exportable as a two-dimensional file in:
  - SVG (`.svg`)
  - DXF (`.dxf`)

### 8.3 Export Location and Filenames

- The user shall select the directory into which exported files are written.
- FoamAirplaneStudio shall automatically propose appropriate descriptive filenames for exported components.
- Default filenames shall identify the corresponding airplane part/body where practical.
- The user shall be able to change the proposed filenames before export.

## 9. Principal Generated Bodies

Depending on the design options selected by the user, the project may generate separate bodies for:

- Left/right wing panels
- Upper/lower portions of each wing panel
- Ailerons
- Flaps
- Fuselage top/bottom/sides depending on split
- Additional user-separated fuselage parts
- Horizontal stabilizer
- Elevator
- Vertical fin
- Rudder
- Other bodies produced by component splits or manufacturing requirements

## 10. Intended Design Philosophy

FoamAirplaneStudio shall emphasize a drawing-driven workflow suitable for model-airplane designers rather than requiring the user to construct the airplane with a general-purpose CAD system.

Where practical, the user shall define aerodynamic shapes, component locations, structural features, and manufacturing intent while the software performs the underlying OCCT surface construction, solid creation, interpolation, Boolean operations, trimming, splitting, mirroring, and mating-geometry generation.

The resulting geometry shall be organized into manufacturable bodies suitable for subsequent CAM processing and CNC routing of foam airplane components.



### Panel station, airfoil and control data
Airfoil Stations, Airfoils and Ailerons/Flaps shall show numbered tabs matching
Outline. Stations and their assignments belong to their outline panel; both
anchors must lie on that panel. Require two stations per panel and assignments
for all stations. Airfoils imported or sketched in any panel shall be selectable
in every panel through one shared library. Each panel shall retain its own
aileron/flap flags, rectangles and hinge styles. Only the selected panel can be
edited, while all committed geometry remains visible. Control rectangles shall
not overlap across panels. Project files preserve all panel data and tab state.
See ADR-0013 for generation and backward migration.

### Wing Lightening
Enable hollowing of the main wing, with user-defined wall thickness, uniformly
spaced crossmembers by count and thickness, first-root start distance and inward
setback before the final station. Split main panels for access. Keep controls
and tips solid; retain wall-thickness support around alignment pins and holes.
Preserve settings and entered units through the project lifecycle.

Regeneration: wing panels run as independent, bounded background tasks. Disable
menus, shortcuts, toolbars, data fields and viewport controls while processing;
show Cancel at the bottom of the data panel. Report panel-specific status and
restore controls on completion, failure or cooperative cancellation. Keep the
last display on cancellation and retain the camera on successful replacement.
The component-job lifecycle is reusable for future fuselage/stabilizer builders.
