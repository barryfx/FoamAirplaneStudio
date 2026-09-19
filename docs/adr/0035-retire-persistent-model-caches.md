# ADR-0035: Retire persistent model caches
Status: Accepted
Date: 2026-09-19

## Context
The user reports that saving generated models takes too long and requests removing
persistence of every model cache, including ignoring embedded models in older files.

## Decision
Write format 23 with no `models` field. Remove geometry serialization, cache decoding
and cache restoration. Accept version-22 project inputs while ignoring `models`
without decompressing, validating checksums or reading BREP shapes. Preserve normal
input validation, atomic saves and the existing file limit. Session caches remain
available until invalidated or the project is closed. Open stays in 2D; explicit
3D/Assembly entry rebuilds models, and Assembly cut intent is retained.

## Alternatives Considered
Keeping persistent caches or introducing sidecars would not meet the request.
Rejecting version-22 files would prevent recovery of existing project inputs.

## Consequences
Saving no longer serializes or round-trip-validates generated geometry. Old large
files still require reading/parsing their JSON; resaving drops the obsolete payload.
Models must regenerate after reopening. No performance improvement is quantified.
This supersedes ADR-0034; mouse and collision behavior are unchanged.

## Validation
Focused persistence/UI checks cover input-only output with populated session
caches, ignored corrupt version-22 models, 2D opening without model jobs, retained
Assembly settings, and model-free resaving. Geometry tests are excluded by request.
