#pragma once
#include <QFileDialog>
#include <QSettings>

namespace designrc::gui {
// Use for every file/folder chooser. Each stable purpose key has its own history.
class FileSelectionDialog : public QFileDialog {
public:
  FileSelectionDialog(QWidget* parent, const QString& purpose, const QString& caption,
      FileMode mode = ExistingFile, AcceptMode acceptMode = AcceptOpen);
private:
  void rememberSelection();
  QSettings settings_;
  QString key_;
};
}
