#pragma once
#include "gui/SketchEditor.h"
#include <optional>
namespace designrc::gui {
// Returns one connected, unbranched, closed boundary with nonzero sampled area.
std::optional<std::vector<QPointF>> closedSketchBoundary(const SketchLayer& layer);
}
