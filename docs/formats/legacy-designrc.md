# Historical DesignRC project format

Copied fixtures use .designrc JSON, format marker DesignRC, version 1.
The old field-based serializer/loader was removed with WingPanelEditor and the
MainWindow wing workflow. The current app does not open or save these legacy files;
File > Open/Save now use `.foam`. Fixtures are retained as historical examples
and for future migration planning.

The replacement `.foam` format is documented in foam-project.md and ADR-0006.
No automatic migration from `.designrc` is currently provided.

Airfoil .dat parsing and STEP/DXF/SVG/PDF utility code remain. These export
utilities are not currently connected to shell actions. STL remains future work.
