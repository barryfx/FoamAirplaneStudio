#include "gui/ReferenceWorkflow.h"
#include "gui/ReferenceImage.h"
#include <QAction>
#include <QToolBar>
#include <cmath>
namespace designrc::gui {
bool referenceReady(const ProjectReference& reference) {
  const auto positive = [](double value) { return std::isfinite(value) && value > 0; };
  if (!reference.toScale)
    return reference.wingspanMm && reference.fuselageLengthMm &&
        positive(*reference.wingspanMm) && positive(*reference.fuselageLengthMm);
  const auto validSize = [&](const std::optional<QSizeF>& size) {
    return size && positive(size->width()) && positive(size->height());
  };
  if (reference.image.empty() || !validSize(reference.image.physicalSizeMm)) return false;
  for (const auto& page : reference.image.pages)
    if (page.pixels.isNull() || !validSize(page.physicalSizeMm)) return false;
  return true;
}
void applyReferenceWorkflow(QToolBar& toolbar, const ProjectReference& reference, bool airfoilsComplete) {
  const bool ready = referenceReady(reference);
  const auto actions = toolbar.actions();
  for (int i = 0; i < actions.size(); ++i)
    actions[i]->setEnabled(i == 0 || (ready && (i == 1 || (i == 2 && airfoilsComplete))));
}
}
