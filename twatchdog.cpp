#include <QDebug>
#include "twatchdog.h"

TWatchDog::TWatchDog(void ExitMainThreadFunc(int))
    : QThread()
{
    WatchDog  = new QTimer(this);
    WatchDog->setSingleShot(true);
    QObject::connect(WatchDog, SIGNAL(timeout()), this, SLOT(onTimeOut()));
    ExitMainThread =ExitMainThreadFunc;
}

TWatchDog::~TWatchDog() {
    WatchDog->deleteLater();
}

void TWatchDog::onTimeOut()
{
    qCritical() << "Watchdog timeout";
    ExitMainThread(-200);
}

void TWatchDog::onStart(int Interval)
{
    WatchDog->setInterval(Interval);
    WatchDog->start();
}

void TWatchDog::onStop()
{
    WatchDog->stop();
}
