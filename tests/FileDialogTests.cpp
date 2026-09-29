#include "gui/FileSelectionDialog.h"
#include <QApplication>
#include <QTemporaryDir>
#include <QFile>
#include <QDir>
#include "TestCheck.h"
#include <iostream>
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
  QTemporaryDir temporary; TEST_CHECK(temporary.isValid());
  QSettings::setDefaultFormat(QSettings::IniFormat);
  QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, temporary.path());
  QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, temporary.path());
  const auto folder = temporary.filePath("artwork"); TEST_CHECK(QDir{}.mkpath(folder));
  const auto file = QDir{folder}.filePath("reference.png");
  { QFile output{file}; TEST_CHECK(output.open(QIODevice::WriteOnly)); output.write("fixture"); }
  {
    TestDialog dialog{nullptr, "referenceImage", "Open"};
    dialog.setOption(QFileDialog::DontUseNativeDialog);
    dialog.selectFile(file); dialog.accept(); TEST_CHECK(dialog.result() == QDialog::Accepted);
  }
  {
    TestDialog dialog{nullptr, "referenceImage", "Open"};
    TEST_CHECK(dialog.directory().absolutePath() == folder);
    TEST_CHECK(!dialog.selectedFiles().isEmpty());
    TEST_CHECK(dialog.selectedFiles().front() == file);
    dialog.selectFile(temporary.filePath("cancelled.png")); dialog.reject();
  }
  {
    TestDialog dialog{nullptr, "referenceImage", "Open"};
    TEST_CHECK(!dialog.selectedFiles().isEmpty());
    TEST_CHECK(dialog.selectedFiles().front() == file); // Cancellation preserves history.
  }
  {
    TestDialog dialog{nullptr, "exportFolder", "Folder", QFileDialog::Directory};
    dialog.setOption(QFileDialog::DontUseNativeDialog);
    TEST_CHECK(dialog.directory().absolutePath() == folder); // Shared first-use fallback.
    dialog.setDirectory(temporary.path()); dialog.selectFile(folder); dialog.accept();
    TEST_CHECK(dialog.result() == QDialog::Accepted);
  }
  {
    TestDialog dialog{nullptr, "exportFolder", "Folder", QFileDialog::Directory};
    TEST_CHECK(dialog.directory().absolutePath() == folder);
  }
  const auto outputPath = QDir{folder}.filePath("part.step");
  {
    TestDialog dialog{nullptr, "exportStep", "Save", QFileDialog::AnyFile, QFileDialog::AcceptSave};
    dialog.setOption(QFileDialog::DontUseNativeDialog);
    dialog.selectFile(outputPath); dialog.accept(); TEST_CHECK(dialog.result() == QDialog::Accepted);
  }
  {
    TestDialog dialog{nullptr, "exportStep", "Save", QFileDialog::AnyFile, QFileDialog::AcceptSave};
    TEST_CHECK(!dialog.selectedFiles().isEmpty());
    TEST_CHECK(dialog.selectedFiles().front() == outputPath);
  }
  TEST_CHECK(QFile::remove(file));
  {
    TestDialog dialog{nullptr, "referenceImage", "Open"};
    TEST_CHECK(dialog.directory().absolutePath() == folder);
  }
  TEST_CHECK(QDir{}.rmdir(folder));
  {
    TestDialog dialog{nullptr, "referenceImage", "Open"};
    TEST_CHECK(dialog.directory().absolutePath() == temporary.path());
  }
  return 0;
}
