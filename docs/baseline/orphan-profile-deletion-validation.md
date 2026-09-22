# Unattached profile deletion validation — 2026-09-22

Edit Profiles permits single-click selection of an unattached profile with an
orange/white highlight. Delete and Delete Profile clear its stable sketch slot
without changing stations. Existing orphan recovery now requires a double-click,
so selecting a profile does not implicitly assign it. Edit Profiles remains
available when sketches exist and no stations remain.

Debug build succeeded. circle_profile_tests, profile_operations_tests and
profile_recovery_tests passed (4.90 seconds total). Focused checks cover clean
selection, rendered orange highlight, keyboard/button deletion, Undo/Redo,
unchanged station records, zero-station access/deletion and double-click recovery.
No component-generation tests ran.

Visually inspected build/debug/orphan-profile-selection.png. Build log:
build/debug/orphan-delete-build.log.
