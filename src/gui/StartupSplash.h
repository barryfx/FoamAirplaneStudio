#pragma once
#include <QElapsedTimer>
#include <QGuiApplication>
#include <QPointer>
#include <QScreen>
#include <QSplashScreen>
#include <QTimer>
#include <algorithm>
namespace designrc::gui {
class StartupSplash final : public QSplashScreen {
public:
  StartupSplash():QSplashScreen{image()} {
    setObjectName("startupSplash");show();repaint();elapsed_.start();
  }
  void finishWhenReady(QWidget& window) {
    // Startup work counts toward the three seconds; keep the event loop live
    // for the remaining time instead of sleeping on the GUI thread.
    const int remaining=static_cast<int>(std::max<qint64>(0,3000-elapsed_.elapsed()));
    QTimer::singleShot(remaining,Qt::PreciseTimer,this,[this,target=QPointer<QWidget>{&window}] {
      if(target){target->showMaximized();finish(target.data());}else close();
    });
  }
protected:
  void mousePressEvent(QMouseEvent*) override {} // Clicking must not dismiss it early.
private:
  static QPixmap image() {
    QSize bounds{512,512};
    if(auto* screen=QGuiApplication::primaryScreen())bounds=bounds.boundedTo(screen->availableGeometry().size()*0.7);
    return QPixmap{":/graphics/FoamAirplaneStudio.png"}.scaled(bounds,Qt::KeepAspectRatio,Qt::SmoothTransformation);
  }
  QElapsedTimer elapsed_;
};
}
