#include "gui/StabilizerAirfoilPanel.h"
#include "gui/AirfoilLibrary.h"
#include "gui/FileSelectionDialog.h"
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <stdexcept>
#include <sstream>
namespace designrc::gui {
namespace {
domain::AirfoilProfile namedProfile(const LibraryAirfoil& entry) {
  // Library names are trimmed; raw DAT headers can retain a CR from CRLF files.
  std::ostringstream dat; dat.precision(17); dat << entry.name.toStdString() << '\n';
  for(auto p:entry.imported->outline()) dat << p.x << ' ' << p.y << '\n';
  std::istringstream stream{dat.str()}; return domain::AirfoilProfile::fromDat(stream);
}
}
domain::AirfoilProfile defaultStabilizerAirfoil() {
  AirfoilLibrary library; QString error;
  if (!library.loadDat(":/airfoils/naca009.dat", error))
    throw std::runtime_error("Cannot load bundled NACA009 airfoil: " + error.toStdString());
  return namedProfile(library.entries().front());
}
StabilizerAirfoilPanel::StabilizerAirfoilPanel(bool horizontal, QWidget* parent)
    : QWidget{parent}, default_{defaultStabilizerAirfoil()} {
  setObjectName(horizontal ? "horizontalStabilizerAirfoilPanel" : "verticalStabilizerAirfoilPanel");
  auto* layout = new QVBoxLayout{this}; layout->setContentsMargins(0,0,0,0);
  auto* text = new QLabel{"Load the airfoil to use for the stabilizer from a .dat file.", this};
  text->setWordWrap(true); layout->addWidget(text);
  auto* button = new QPushButton{"Load Airfoil .dat File", this};
  button->setObjectName("loadStabilizerAirfoil"); layout->addWidget(button);
  name_ = new QLabel{this}; name_->setObjectName("stabilizerAirfoilName"); name_->setWordWrap(true);
  name_->setTextFormat(Qt::PlainText); layout->addWidget(name_); layout->addStretch();
  restore({});
  connect(button, &QPushButton::clicked, this, [this] {
    FileSelectionDialog dialog{this, "stabilizerAirfoilDat", "Load Airfoil .dat File"};
    dialog.setNameFilter("Airfoil coordinates (*.dat);;All files (*)");
    if (dialog.exec() != QDialog::Accepted) return;
    QString error;
    if (!loadFile(dialog.selectedFiles().front(), error)) QMessageBox::warning(this, "Load Airfoil", error);
  });
}
void StabilizerAirfoilPanel::restore(std::optional<domain::AirfoilProfile> profile) {
  selected_ = std::move(profile); name_->setText(QString::fromStdString(airfoil().name()));
}
bool StabilizerAirfoilPanel::loadFile(const QString& path, QString& error) {
  AirfoilLibrary library;
  if (!library.loadDat(path, error)) return false;
  restore(namedProfile(library.entries().front()));
  if (changed) changed();
  return true;
}
}
