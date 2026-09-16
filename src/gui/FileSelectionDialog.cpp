#include "gui/FileSelectionDialog.h"
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

namespace designrc::gui {
FileSelectionDialog::FileSelectionDialog(QWidget* parent, const QString& purpose,
    const QString& caption, FileMode mode, AcceptMode acceptMode)
    : QFileDialog{parent, caption}, key_{"fileDialogs/" + purpose + "/selection"} {
  connect(this, &QDialog::accepted, this, [this] { rememberSelection(); });
  setFileMode(mode);
  setAcceptMode(acceptMode);
  const QString previous = settings_.value(key_).toString();
  const QFileInfo info{previous};
  if (!previous.isEmpty() && info.exists()) {
    setDirectory(info.isDir() ? info.absoluteFilePath() : info.absolutePath());
    if (!info.isDir() && mode != Directory) selectFile(info.absoluteFilePath());
    return;
  }
  // Deleted/moved targets fall back to their nearest surviving parent.
  QString directory = previous.isEmpty()
      ? settings_.value("fileDialogs/lastDirectory").toString() : info.absolutePath();
  while (!directory.isEmpty() && !QFileInfo{directory}.isDir()) {
    const QString parentDirectory = QFileInfo{directory}.absolutePath();
    if (parentDirectory == directory) { directory.clear(); break; }
    directory = parentDirectory;
  }
  if (directory.isEmpty()) directory = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
  if (directory.isEmpty()) directory = QDir::homePath();
  setDirectory(directory);
  if (acceptMode == AcceptSave && !previous.isEmpty()) selectFile(info.fileName());
}
void FileSelectionDialog::rememberSelection() {
  if (selectedFiles().isEmpty()) return;
  const QFileInfo selection{selectedFiles().front()};
  settings_.setValue(key_, selection.absoluteFilePath());
  settings_.setValue("fileDialogs/lastDirectory",
      fileMode() == Directory ? selection.absoluteFilePath() : selection.absolutePath());
  settings_.sync();
}
}
