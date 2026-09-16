#pragma once
#include <QApplication>
#include <QEventLoop>
#include <QTimer>
#include <QWidget>
#include <stdexcept>
inline void waitForModel(QWidget& window) {
  QApplication::processEvents();
  if(!window.property("modelProcessing").toBool())return;
  QEventLoop loop;QTimer poll,timeout;bool expired=false;
  QObject::connect(&poll,&QTimer::timeout,&loop,[&]{if(!window.property("modelProcessing").toBool())loop.quit();});
  QObject::connect(&timeout,&QTimer::timeout,&loop,[&]{expired=true;loop.quit();});
  poll.start(20);timeout.setSingleShot(true);timeout.start(600000);loop.exec();
  if(expired)throw std::runtime_error("Timed out waiting for background model processing.");
}
