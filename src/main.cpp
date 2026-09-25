#include "gui/MainWindow.h"
#include "gui/StartupSplash.h"

#include <QApplication>
#include <QDir>
#include <QFileInfo>
#include <QIcon>
#include <QtGlobal>

int main(int argc, char* argv[]) {
#if defined(Q_OS_WIN)
  // A Debug build may be launched from the project or out directory rather
  // than with the executable directory as its working directory. Point Qt at
  // the platform plug-in deployed beside the executable before QApplication
  // begins plug-in discovery.
  if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM_PLUGIN_PATH") && argc > 0) {
    const QDir executableDirectory{QFileInfo{QString::fromLocal8Bit(argv[0])}.absolutePath()};
    const QString platformDirectory = executableDirectory.filePath("platforms");
    if (QFileInfo::exists(QDir{platformDirectory}.filePath("qwindowsd.dll")) ||
        QFileInfo::exists(QDir{platformDirectory}.filePath("qwindows.dll")))
      qputenv("QT_QPA_PLATFORM_PLUGIN_PATH", platformDirectory.toLocal8Bit());
  }
#endif
#if defined(Q_OS_LINUX)
  // OCCT's Xw_Window embeds into an X11 window. WSLg and Wayland desktops
  // provide that window through XWayland when Qt uses its XCB backend.
  if (qEnvironmentVariableIsEmpty("QT_QPA_PLATFORM"))
    qputenv("QT_QPA_PLATFORM", "xcb");
#endif
  QApplication application{argc, argv};
  QApplication::setStyle("Fusion");
  application.setApplicationName("FoamAirplaneStudio");
  application.setApplicationVersion(DESIGNRC_VERSION);
  application.setOrganizationName("FoamAirplaneStudio");
  application.setWindowIcon(QIcon(":/graphics/FoamAirplaneStudio.png"));

  designrc::gui::StartupSplash splash;
  designrc::gui::MainWindow window;
  splash.finishWhenReady(window);
  return application.exec();
}


