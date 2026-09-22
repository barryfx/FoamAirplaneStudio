#pragma once
#include "geometry/ComponentExporter.h"
#include "gui/ComponentNames.h"
#include <QSet>
#include <QWidget>
#include <functional>
class QVBoxLayout;
class QLabel;
namespace designrc::gui {
class InspectPanel final : public QWidget {
public:
  explicit InspectPanel(QWidget* parent=nullptr);
  void setParts(std::vector<geometry::ExportPart> parts);
  void restore(ComponentNames names,bool preserveVisibility=false);
  const ComponentNames& names() const {return names_;}
  std::vector<TopoDS_Shape> visibleShapes() const;
  std::function<void()> namesChanged,visibilityChanged;
private:
  ComponentNames names_;
  QSet<QString> hidden_;
  std::vector<geometry::ExportPart> parts_;
  QVBoxLayout* list_{};
  QLabel* message_{};
};
}
