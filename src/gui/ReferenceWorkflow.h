#pragma once
class QToolBar;
namespace designrc::gui {
struct ProjectReference;
bool referenceReady(const ProjectReference& reference);
void applyReferenceWorkflow(QToolBar& toolbar, const ProjectReference& reference, bool airfoilsComplete=false);
}
