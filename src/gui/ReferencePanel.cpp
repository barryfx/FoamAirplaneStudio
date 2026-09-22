#include "gui/ReferencePanel.h"
#include "gui/ProcessingScope.h"
#include <QButtonGroup>
#include <QComboBox>
#include "gui/LengthEntry.h"
#include "gui/FileSelectionDialog.h"
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QRadioButton>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include <cmath>

namespace designrc::gui {
ReferencePanel::ReferencePanel(QWidget* parent) : QWidget{parent} {
  auto* layout = new QVBoxLayout{this};
  layout->setContentsMargins(0, 0, 0, 0);
  auto* load = new QPushButton{"Load Image", this};
  load->setObjectName("loadReferenceImage");
  layout->addWidget(load);
  path_ = new QLabel{this};
  path_->setObjectName("referencePath");
  path_->setWordWrap(true);
  path_->setTextFormat(Qt::PlainText);
  path_->setTextInteractionFlags(Qt::TextSelectableByMouse);
  layout->addWidget(path_);
  auto* group = new QButtonGroup{this};
  toScale_ = new QRadioButton{"User Reference Image Scale", this};
  toScale_->setObjectName("referenceToScale");
  specify_ = new QRadioButton{"Specify Dimensions", this};
  specify_->setObjectName("specifyDimensions");
  group->addButton(toScale_);
  group->addButton(specify_);
  layout->addWidget(toScale_);
  layout->addWidget(specify_);
  size_ = new QLabel{this};
  size_->setWordWrap(true);
  layout->addWidget(size_);
  dimensions_ = new QWidget{this};
  auto* fields = new QFormLayout{dimensions_};
  fields->setContentsMargins(0, 0, 0, 0);
  fields->setRowWrapPolicy(QFormLayout::WrapLongRows);
  units_ = new QComboBox{dimensions_};
  units_->setObjectName("projectUnits");
  units_->addItems({"Millimeters", "Inches"});
  fields->addRow("Project Units", units_);
  auto addLength = [&](const char* label, const char* name) {
    auto* edit = new QLineEdit{dimensions_};
    edit->setObjectName(name);
    edit->setPlaceholderText("e.g. 25.4 mm or 1 in");
    edit->setToolTip("Positive decimal; optional mm or in suffix. Bare numbers use Project Units.");
    fields->addRow(label, edit);
    connect(edit, &QLineEdit::textChanged, this, [this] { updateDimensions(); });
    return edit;
  };
  wingspan_ = addLength("Wingspan", "referenceWingspan");
  layout->addWidget(dimensions_);
  layout->addStretch();
  connect(load, &QPushButton::clicked, this, [this] { loadImage(); });
  connect(toScale_, &QRadioButton::toggled, this, [this] { updateMode(); });
  connect(units_, &QComboBox::currentIndexChanged, this, [this] { changeUnits(); });
  reset();
}
void ReferencePanel::reset() {
  reference_ = {};
  const QSignalBlocker blockScale{toScale_}, blockUnits{units_};
  specify_->setChecked(true);
  toScale_->setEnabled(false);
  units_->setCurrentIndex(0);
  path_->setText("No reference loaded");
  refreshDimensionFields();
  updateMode();
}
void ReferencePanel::restoreReference(const ProjectReference& reference) {
  reference_=reference;
  const QSignalBlocker scaleBlock{toScale_},unitsBlock{units_};
  toScale_->setEnabled(reference.image.physicalSizeMm.has_value());
  toScale_->setChecked(reference.toScale); specify_->setChecked(!reference.toScale);
  units_->setCurrentIndex(reference.units==ProjectUnits::Inches?1:0);
  path_->setText(reference.image.path.isEmpty()?"No reference loaded":reference.image.path);
  path_->setToolTip(reference.image.path); refreshDimensionFields(); updateMode();
}
void ReferencePanel::loadImage() {
  FileSelectionDialog dialog{this, "referenceImage", "Load Reference Image"};
  dialog.setNameFilter("Reference files (*.png *.jpg *.jpeg *.pdf);;PNG (*.png);;JPEG (*.jpg *.jpeg);;PDF (*.pdf)");
  if (dialog.exec() != QDialog::Accepted) return;
  const QString path = dialog.selectedFiles().front();
  QString error;
  ReferenceImage image;
  {
    ProcessingScope processing{this, "Loading reference image / rendering PDF pages..."};
    image = loadReferenceImage(path, error);
    if (image.empty()) processing.update("Reference load failed: " + error);
  }
  if (image.empty()) { QMessageBox::warning(this, "Load Image", error); return; }
  ProcessingScope processing{this, "Displaying reference pages..."};
  reference_.image = std::move(image);
  path_->setText(reference_.image.path);
  path_->setToolTip(reference_.image.path);
  toScale_->setEnabled(reference_.image.physicalSizeMm.has_value());
  if (!toScale_->isEnabled()) specify_->setChecked(true);
  updateMode();
  processing.update(QString{"Loaded reference: %1 page(s)"}.arg(reference_.image.pages.size()));
}
void ReferencePanel::refreshDimensionFields() {
  const QSignalBlocker spanBlock{wingspan_};
  const double factor = reference_.units == ProjectUnits::Inches ? 25.4 : 1.0;
  auto text = [factor](std::optional<double> mm) {
    return mm ? QLocale::c().toString(*mm / factor, 'g', 12) : QString{};
  };
  wingspan_->setText(text(reference_.wingspanMm));
}
void ReferencePanel::changeUnits() {
  // Make implicit old units explicit before changing the project default.
  for(auto* edit:{wingspan_}) {
    const QSignalBlocker block{edit};edit->setText(explicitLength(edit->text(),reference_.units));
  }
  reference_.units = units_->currentIndex() == 1 ? ProjectUnits::Inches : ProjectUnits::Millimeters;
  emit referenceChanged();
}
void ReferencePanel::updateDimensions() {
  reference_.wingspanMm = lengthInMm(wingspan_->text(),reference_.units);
  emit referenceChanged();
}
void ReferencePanel::updateMode() {
  reference_.toScale = toScale_->isChecked() && reference_.image.physicalSizeMm.has_value();
  dimensions_->setVisible(!reference_.toScale);
  if (reference_.toScale) {
    for(auto* edit:{wingspan_}) {
      const QSignalBlocker block{edit};edit->setText(explicitLength(edit->text(),reference_.units));
    }
    reference_.units = reference_.image.nativeUnits;
    const QSignalBlocker block{units_};
    units_->setCurrentIndex(reference_.units == ProjectUnits::Inches ? 1 : 0);
    const double divisor = reference_.units == ProjectUnits::Inches ? 25.4 : 1.0;
    const auto size = *reference_.image.physicalSizeMm / divisor;
    size_->setText(QString{"Reference bounds: %1 × %2 %3"}
        .arg(size.width(), 0, 'g', 8).arg(size.height(), 0, 'g', 8)
        .arg(reference_.units == ProjectUnits::Inches ? "inches" : "mm"));
  } else {
    size_->setText(reference_.image.empty() ? QString{} :
        reference_.image.physicalSizeMm ? "Enter Wingspan in the selected project units to scale all project lengths." :
        "This image has no physical-size metadata. Specify Wingspan to scale all project lengths.");
  }
  emit referenceChanged();
}
}

