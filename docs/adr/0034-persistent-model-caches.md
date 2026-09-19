# ADR-0034: Persist generated model caches
Status: Superseded
Date: 2026-09-19

## Context
The user requests storing every cached 3D model to avoid regeneration after opening.
Version 21 persists inputs and Assembly placement/cut intent but not generated shapes.

## Decision
Version 22 adds optional model groups to the existing atomic JSON project file.
Store OCCT BREP text including triangulations/normals, compressed with qCompress and
base64 encoded with SHA-256 integrity checks. Preserve all component result roles,
fuselage accessories, and Assembly originals/cut results. Input fingerprints guard
cache reuse; Assembly additionally binds placement/cut state. Remain in 2D on Open.
Exclude geometry blobs from input and dirty fingerprints. Retain readers for older
versions; missing/stale models build only when explicitly entering 3D.

## Alternatives Considered
External sidecar caches are smaller main files but can become separated from them.
Mesh-only storage cannot support subsequent OCCT cuts or exact geometry export.
Regeneration from inputs does not meet the requested opening behavior.

## Consequences
Files are larger and saving/loading includes serialization cost. Repeated shapes
may be duplicated across groups. Size limits, checksums and BREP parsing reject
corrupt files transactionally. Future incompatible geometry changes must invalidate
cache fingerprints or bump the cache/file format. Source inputs remain authoritative.
No new external dependency is added. Older applications reject format 22.

## Validation
A focused cache/UI test uses a hand-authored triangulated face, checking all cache
roles, original/cut restoration, stale rejection, corrupt checksum rejection,
legacy inputs, and mouse controls. No geometry tests are run for this prompt.

Superseded by ADR-0035 at user request because saving geometry takes too long.
