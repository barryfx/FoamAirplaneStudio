#pragma once
#include "geometry/Assembly.h"
#include <filesystem>
namespace designrc::domain { struct PartDrawing; }

namespace designrc::geometry {
struct ExportPart {
  std::string name;
  TopoDS_Shape shape;
  std::optional<gp_Pln> formerPlane;
};
// Formers first, in their already assigned nose-to-tail order; each other
// manufacturing solid gets its own selectable entry.
std::vector<ExportPart> assemblyExportParts(const AssemblyParts& assembly);
domain::PartDrawing formerDrawing(const ExportPart& part);
enum class FormerExportFormat { Dxf, Stl, Step };
enum class ComponentExportFormat { Step, Stl };
std::vector<std::string> exportFileNames(const std::vector<ExportPart>& selected,
    FormerExportFormat formers, ComponentExportFormat components);
void writeComponentExports(const std::vector<ExportPart>& selected,
    FormerExportFormat formers, ComponentExportFormat components,
    const std::filesystem::path& directory);
}
