#pragma once

#include "geometry/OcctRibBuilder.h"

#include <filesystem>
#include <string>
#include <vector>

namespace designrc::geometry {
enum class StepAssemblyLayout { LegacyWing, Aircraft };

void exportStepAssembly(const std::vector<NamedPartShape>& parts,
                        const std::filesystem::path& path,
                        const std::string& assemblyName,
                        StepAssemblyLayout layout = StepAssemblyLayout::LegacyWing);

} // namespace designrc::geometry
