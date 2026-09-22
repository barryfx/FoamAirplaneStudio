# Editor history validation — 2026-09-22

Debug application and focused test targets built successfully with MSVC/Qt.

Passed: editor_history_tests, sketch_editor_tests, circle_profile_tests,
profile_operations_tests, station_sketch_tests, control_surface_editor_tests.
No Wing/Fuselage generation tests or broad regression suite were run.

The new GUI history suite checks menu shortcuts, actual keyboard undo/redo,
line/spline/circle highlighting, individual curve deletion, one-revision drags,
click-to-drop station movement, attached station restoration on curve undo,
all twelve sketch collections, control surfaces, formers, servo trays, RC part
creation/movement/deletion, Assembly offsets, component names, saved-state
tracking, redo invalidation and New history reset. No component generation jobs
are started. Existing profile tests cover copy/paste/move and circle editing.

Renderer capture: build/debug/editor-history.png (selected orange/white line,
unselected blue spline/circle); visually inspected. GUI events and viewport
painting are also exercised by the automated checks.

Build logs: build/debug/editor-history-build.log and editor-history-test-build.log.
Debug executable: build/debug/Debug/foamairplanestudio.exe.
