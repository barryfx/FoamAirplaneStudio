# Open-root wing generation correction

Date: 2026-09-13. Windows Debug, Qt 6.11.1, OCCT 8.

The reported "The outline closes before the wing tip" error was reproduced
with an otherwise valid outline whose open root endpoints were offset by eight
scene units. The first airfoil station was slightly outboard. Previously, the
station established the section direction and the minimum outline projection
established the root. That produced a single intersection at one open endpoint,
which was incorrectly treated as an interior zero-chord section.

The root now comes from the first panel's open root endpoints. Temporary straight
panel closures support intersection queries without modifying the user's sketch.
Sections stay on the outboard side of that root plane. Genuine interior pinches
remain errors. ADR-0005 and the wing-solids architecture document were updated.

Regression coverage includes offset root endpoints, an angled first station,
interior pinch rejection and the existing scale/mirror/tip/panel cases.
The MainWindow workflow fixture also uses an offset open root and exercises
generation and tip regeneration through the actual UI controls.

The original application session remains open; its executable was renamed before
linking to preserve unsaved work. DesignRC was not modified. Linux/macOS were not
tested. Build log: build/debug/root-fix-build.log.

Validation passed: normal Debug build; wing_solid_tests and
station_workflow_tests (2/2, 48.67 seconds). The captured viewport at
build/debug/root-fix-smoke.png shows the mirrored solid with the offset-root
fixture. The rebuilt normal Debug app was launched successfully.
