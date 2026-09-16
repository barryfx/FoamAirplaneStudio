# AGENTS.md --- FoamDesign Codex SOP

## Mission

**FoamAirplaneDesign** Uses OCCT to enable the user to design
foam airplanes.  It exports .step or STL files of parts to be
CNC routed or 3D printed, and .svg or .dxf outline files of
parts to be laser cut.

## Mandatory session startup

Before substantial work: Read `AGENTS.md` and any relavent
documentation in the project directory tree.  Repository documentation
overrides remembered chat context.

## Git SOP

### Before edits

-   Inspect `git status`; never discard unrelated user work.
-   Never use destructive reset/clean commands merely to get a clean
    tree.
-   Inspect overlapping pre-existing changes before modifying them.

### Commits

-   Make cohesive commits, one logical change per commit when practical.
-   Use imperative messages, e.g. `Add Vulkan device initialization`.
-   Do not commit build output, SDKs, credentials or large generated
    artifacts unless policy requires it.
-   Run relevant tests/validation before commit.
-   Record benchmark evidence before committing performance claims.

### History safety

Do not force-push, rewrite shared history, delete branches, or
reset/revert user work without explicit instruction. Prefer targeted
edits or normal reverts when backing out project changes.

## ADR SOP

Document design choices in an ADR.
Store ADRs in `docs/adr/` as `NNNN-short-name.md`.

Template:

``` text
# ADR-NNNN: Title
Status: Proposed | Accepted | Superseded | Rejected
Date: YYYY-MM-DD

## Context
## Decision
## Alternatives Considered
## Consequences
## Validation
```

Create an ADR for expensive-to-reverse choices.

## Documentation SOP

Documentation is part of implementation. Update as applicable: -
`docs/architecture/` for current design. - `docs/formats/` for
persistent/interchange formats. -`docs/adr/` for architectural 
decisions. Do not leave documentation knowingly inconsistent with code.

## Coding SOP

-   Create cross-platform code.  Targets are Windows, Linux and MacOS.
-   Prefer clear C++23 with explicit ownership/lifetimes.
-   Keep platform-specific code isolated.
-   Avoid hidden global state.
-   Do not optimize unreadable code without measured justification.
-   Add comments for non-obvious synchronization, memory-layout and GPU
    assumptions.
-   Avoid unnecessary dependencies; justify substantial new ones.

## Testing/validation

For future implementation requests, unless the user instructs otherwise, run
the appropriate tests, rebuild the Debug application, and launch the rebuilt
Debug application. If the app is running, kill its workspace executable processes
before building over it, as explicitly requested by the user. Preserve unrelated
user work and processes.

For every substantive change, run the smallest relevant set of
build/tests plus validation. Renderer changes should include
visual/smoke validation; performance changes require benchmark evidence.
If a test cannot run, document why rather than claiming success.

## Codex autonomy boundaries

Codex may implement, test, refactor locally, update docs and propose
experiments within the current phase. Codex must not silently choose a
new major architecture, change project goals, replace OCCT,
add major dependencies, change persistent formats
incompatibly, or skip benchmark parity requirements. Such changes
require discussion and normally an ADR.
