#ifndef TWATCHDOG_H
#define TWATCHDOG_H

#include <QThread>
#include <QObject>
#include <QTimer>

class TWatchDog : public QThread
{
    Q_OBJECT
private:
    QTimer *WatchDog;
    void (*ExitMainThread)(int);

public:
    explicit TWatchDog(void ExitMainThreadFunc(int));
    ~TWatchDog();

private slots:
    void onTimeOut();

public slots:
    void onStart(int Interval);
    void onStop();
};

#endif // TWATCHDOG_H
