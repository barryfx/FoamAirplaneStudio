#pragma once
#include "gui/ReferenceImage.h"
#include <QWidget>

class QLabel;
class QRadioButton;
class QComboBox;
class QLineEdit;

namespace designrc::gui {
class ReferencePanel final : public QWidget {
  Q_OBJECT
public:
  explicit ReferencePanel(QWidget* parent = nullptr);
  const ProjectReference& projectReference() const { return reference_; }
  void reset();
  void restoreReference(const ProjectReference& reference);
signals:
  void referenceChanged();
private:
  void loadImage();
  void updateMode();
  void updateDimensions();
  void changeUnits();
  void refreshDimensionFields();
  ProjectReference reference_;
  QLabel* path_{};
  QLabel* size_{};
  QRadioButton* toScale_{};
  QRadioButton* specify_{};
  QWidget* dimensions_{};
  QComboBox* units_{};
  QLineEdit* wingspan_{};
};
}
