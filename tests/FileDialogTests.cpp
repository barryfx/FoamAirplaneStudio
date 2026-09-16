#include "gui/FileSelectionDialog.h"
#include <QApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include <cassert>
#include <iostream>
#undef assert
#define assert(condition) do { if (!(condition)) { std::cerr << "Check failed at line " << __LINE__ << ": " << #condition << std::endl; return 1; } } while (false)
using namespace designrc::gui;
class TestDialog : public FileSelectionDialog {
public:
  using FileSelectionDialog::FileSelectionDialog;
  using FileSelectionDialog::accept;
};
int main(int argc, char** argv) {
  QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
  qInstallMessageHandler([](QtMsgType, const QMessageLogContext&, const QString& message) {
    std::cerr << message.toStdString() << std::endl;
  });
  QApplication app{argc, argv};
  app.setOrganizationName("FoamDialogTests"); app.setApplicationName("Isolated");
  QTemporaryDir temporary; assert(temporary.isValid());
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, temporary.path());
  QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, temporary.path());
  const auto folder = temporary.filePath("artwork"); assert(QDir{}.mkpath(folder));
  const auto file = QDir{folder}.filePath("reference.png");
  { QFile output{file}; assert(output.open(QIODevice::WriteOnly)); output.write("fixture"); }
  {
    TestDialog dialog{nullptr, "referenceImage", "Open"};
    dialog.setOption(QFileDialog::DontUseNativeDialog);
    dialog.selectFile(file); dialog.accept(); assert(dialog.result() == QDialog::Accepted);
  }
  {
    TestDialog dialog{nullptr, "referenceImage", "Open"};
    assert(dialog.directory().absolutePath() == folder);
    assert(!dialog.selectedFiles().isEmpty());
    assert(dialog.selectedFiles().front() == file);
    dialog.selectFile(temporary.filePath("cancelled.png")); dialog.reject();
  }
  {
    TestDialog dialog{nullptr, "referenceImage", "Open"};
    assert(!dialog.selectedFiles().isEmpty());
    assert(dialog.selectedFiles().front() == file); // Cancellation preserves history.
  }
  {
    TestDialog dialog{nullptr, "exportFolder", "Folder", QFileDialog::Directory};
    dialog.setOption(QFileDialog::DontUseNativeDialog);
    assert(dialog.directory().absolutePath() == folder); // Shared first-use fallback.
    dialog.setDirectory(temporary.path()); dialog.selectFile(folder); dialog.accept();
    assert(dialog.result() == QDialog::Accepted);
  }
  {
    TestDialog dialog{nullptr, "exportFolder", "Folder", QFileDialog::Directory};
    assert(dialog.directory().absolutePath() == folder);
  }
  const auto outputPath = QDir{folder}.filePath("part.step");
  {
    TestDialog dialog{nullptr, "exportStep", "Save", QFileDialog::AnyFile, QFileDialog::AcceptSave};
    dialog.setOption(QFileDialog::DontUseNativeDialog);
    dialog.selectFile(outputPath); dialog.accept(); assert(dialog.result() == QDialog::Accepted);
  }
  {
    TestDialog dialog{nullptr, "exportStep", "Save", QFileDialog::AnyFile, QFileDialog::AcceptSave};
    assert(!dialog.selectedFiles().isEmpty());
    assert(dialog.selectedFiles().front() == outputPath);
  }
  assert(QFile::remove(file));
  {
    TestDialog dialog{nullptr, "referenceImage", "Open"};
    assert(dialog.directory().absolutePath() == folder);
  }
  assert(QDir{}.rmdir(folder));
  {
    TestDialog dialog{nullptr, "referenceImage", "Open"};
    assert(dialog.directory().absolutePath() == temporary.path());
  }
  return 0;
}
