# Documentation audit
Date: 2026-09-18
Source revision: ab24602 (before these documentation corrections)

## Scope

Read AGENTS.md and the docs tree: requirements, GUI design, current architecture,
format specifications, ADRs and historical validation/provenance. Cross-checked
current claims against CMakeLists.txt, CMakePresets.json, ProjectDocument.cpp,
MainWindow.cpp, ReferenceWorkflow.cpp, component builders and relevant test entry
points. Also checked the root README and bundled HTML help for consistency.

## Corrections

- Current saves use format 20 and read versions 1-19. Added missing hinge/Cut Shape
  fields and station thickness to the format overview; retained introduction
  versions in the migration history.
- Stabilizer hinges, closed Cut Shapes, horizontal centerline joining and
  successful-result caching are implemented. Assembly/interfaces, export actions
  and undo remain future work.
- Fuselage and stabilizer manual scaling are implemented. Stabilizers share the
  Fuselage Side View scale.
- Requirements now reflect the accepted left/right main-body split, whole
  cut-outs/inserts, four alignment pins and former retaining rails (ADR-0029/0032).
  These correct superseded wording; no new design decision was made.
- Complete Fuselage generation initializes missing station walls without a
  Thicken visit. Defaults use wing-LE position, not fuselage length. Updated the
  documented tray/retainer/cut/split/assembly sequence.
- The Windows preset uses Visual Studio 18 2026, whose generator was introduced
  in CMake 4.2, confirmed in the installed CMake generator documentation. The
  generic CMakeLists minimum remains 3.24; preset schema 6 requires 3.25, but
  neither lower number suffices for this Windows generator.
- README test guidance now uses an explicit non-generation subset and points to
  AGENTS.md instead of suggesting an unrestricted test run.
- Repaired invalid Windows-1252 range dashes in otherwise UTF-8 documentation.

## Validation and limits

Documentation was checked against source; this is not a new geometry, performance
or platform validation. Historical ADR decisions and baseline measurements retain
their original context. Archived DesignRC documents and the source manifest are
provenance, not current application instructions or current-file checksums.

No application code or project format was changed. No tests, benchmarks, Debug
rebuild or application launch were needed for this documentation audit. Wing and
Fuselage generation were not run. The bundled Help source correction will reach
the deployed Help file on the next application build.

Checks passed: strict UTF-8 decoding of all documentation/instruction files,
13 local Markdown links outside the archived DesignRC documents, and
`git diff --check`. CTest's `-N` listing confirmed that the documented filter
selects exactly reference_tests, sketch_editor_tests and wing_workflow_tests;
this listed tests without executing them.
