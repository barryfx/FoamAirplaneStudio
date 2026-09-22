# Orphan profile recovery validation — 2026-09-22

Added direct recovery in Edit Profiles: select an unassigned station, turn drawing
tools off, and click an orphaned profile boundary or closed interior. Reuses the
existing slot without duplicating/moving geometry. Assigned profiles cannot be
stolen and existing station assignments cannot be replaced by an ordinary click.

Debug build succeeded. Relevant tests passed:
- circle_profile_tests
- profile_operations_tests
- profile_recovery_tests

Recovery checks move retention (including an outside-outline hover/drop), deleting
and recreating a station, retained orphan geometry, reattachment, Ctrl+Z/Ctrl+Y,
save-point state, occupied-profile protection and save/reopen persistence.
No component-generation tests ran. The intermittent station disappearance was
not reproduced; no claim is made that its underlying cause has been fixed.

Build log: build/debug/profile-recovery-build.log.
