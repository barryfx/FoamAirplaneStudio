#pragma once
#include <QApplication>
#include <QMainWindow>
#include <QStatusBar>

namespace designrc::gui {
// Paint status synchronously without dispatching input or re-entering document edits.
// Qt's cursor stack also supports nested operations and exception unwinding.
class ProcessingScope {
public:
  ProcessingScope(QWidget* owner, const QString& message)
      : window_{qobject_cast<QMainWindow*>(owner->window())} {
    QApplication::setOverrideCursor(Qt::WaitCursor);
    update(message);
  }
  ~ProcessingScope() { QApplication::restoreOverrideCursor(); }
  ProcessingScope(const ProcessingScope&) = delete;
  ProcessingScope& operator=(const ProcessingScope&) = delete;
  void update(const QString& message) {
    if (window_) { window_->statusBar()->showMessage(message); window_->statusBar()->repaint(); }
  }
private:
  QMainWindow* window_;
};
}
