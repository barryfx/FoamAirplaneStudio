# ADR-0027: Independent stabilizer outlines and a single airfoil workflow
Status: Accepted
Date: 2026-09-17

## Context
Both stabilizers need the Wing sketch interaction, independent saved outlines,
and one airfoil type per component instead of station-based assignments.

## Decision
Use one existing SketchEditor per stabilizer and one shared outline panel class.
Expose Outline, Airfoil, Hinge Line and Cut. Implement Outline now. Store both
sketch states in project version 16; migrate versions 1–15 to empty outlines and
map removed toolbar selections to Outline (Airfoils maps to Airfoil).

Outline is an open chain. Following the revised orientation requirement, warn at
mode/viewport exit when the line through its endpoints is more than 10 degrees
from both horizontal and vertical, or the endpoints coincide. This replaces the
original 2% height check and supports drawings rotated 90 degrees. Also warn about nonempty
disconnected, branched or closed paths. Retain incomplete geometry on warning and
in saved files. Horizontal instructions explain right-half mirroring; actual
stabilizer solids, mirroring, airfoil selection and cuts remain future work.

## Alternatives Considered
Reusing Wing's data would couple unrelated components and station assignments.
A pixel-based warning would vary with zoom. Rejecting imperfect sketches at save
would prevent retaining unfinished work.

## Consequences
Version 16 requires an updated reader, while this reader retains older-format
support. No new dependencies or geometry architecture is introduced. Airfoil
data and solid generation will need a later design appropriate to one profile.

## Validation
stabilizer_outline_tests covers both component interactions, warnings, geometry
isolation, reference remapping, malformed files, migration and project lifecycle.
Run results are recorded in docs/baseline/stabilizer-outline-validation.md.
