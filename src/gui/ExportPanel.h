#pragma once
#include "geometry/ComponentExporter.h"
#include <QWidget>
#include <functional>
class QCheckBox;
class QRadioButton;
class QPushButton;
class QVBoxLayout;
namespace designrc::gui {
class ExportPanel final : public QWidget {
public:
  explicit ExportPanel(QWidget* parent=nullptr);
  void setParts(std::vector<geometry::ExportPart> parts);
  std::vector<geometry::ExportPart> selectedParts() const;
  geometry::FormerExportFormat formerFormat() const;
  geometry::ComponentExportFormat componentFormat() const;
  std::function<void()> exportRequested;
private:
  void updateSelection();
  std::vector<geometry::ExportPart> parts_;
  std::vector<QCheckBox*> checks_;
  QCheckBox* all_{};
  QRadioButton *dxf_{},*formerSvg_{},*formerStep_{},*step_{};
  QPushButton* export_{};
  QVBoxLayout* list_{};
};
}
