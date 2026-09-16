# ADR-0001: Reference scale and Qt PDF loading
Status: Accepted
Date: 2026-09-13

## Context
The Reference workflow must load PNG/JPG/PDF and distinguish actual-size drawings
from drawings whose aircraft dimensions are supplied by the user. Qt Widgets
already provides the application UI and raster decoding; PDF decoding was missing.

## Decision
Use Qt PDF (QPdfDocument) for PDF page rasterization and page size. Install the
matching Qt PDF add-on, not a browser engine or an external rendering process.
Keep reference pixels, optional physical page dimensions, project units, and
optional aircraft dimensions in one per-window ProjectReference state. Physical
lengths use millimeters internally; inches are a display/input conversion.
No persistent file format is established by this in-memory state.

Only explicit PNG pHYs or JPEG EXIF/JFIF density establishes raster physical size.
PNG meters and JPEG centimeters display as millimeters; JPEG inch metadata displays
as inches. PDF points are 1/72 inch and display as inches. This is a physical-unit
conversion, not an inference about units printed in the artwork.
Missing metadata disables actual-size mode rather than accepting Qt's fallback DPI.
PDF rendering is capped at 4096 pixels on its longest side and 144 DPI, independently
of physical page dimensions. All PDF pages load in document order as separate image items in a vertical stack. Aggregate rasterization is capped at 32 million pixels, retaining a common scale across pages.

## Alternatives Considered
OS-specific PDF APIs would undermine portability. External PDF executables would
add deployment/process dependencies. Qt's default raster DPI is not reliable
evidence of physical scale.

## Consequences
Qt PDF is a required matching Qt module on each platform. The Windows SDK was
supplemented with Qt PDF 6.11.1 from Qt's official repository; its original extracted
add-on is also in sibling third_party/qt-pdf-6.11.1. Existing SDK files were checked
and not replaced. windeployqt deploys Qt PDF and its runtime dependencies.
Manual aircraft dimensions do not imply that the full image bounds are the
aircraft bounds: margins/multiple views require later traced-outline calibration.
Unscaled previews currently use pixel scene coordinates. Actual-size previews use
millimeter scene coordinates. Reference images remain in the 2D view only.

## Validation
Windows Debug builds and launch passed. The running app displayed a loaded PDF
plan and the requested controls. Added metadata/unit-conversion coverage is
compiled only; automated tests were not run under the user's existing instruction.

