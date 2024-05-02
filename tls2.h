#ifndef TLS2_H
#define TLS2_H

//Qt
#include <QObject>
#include <QtNetwork/QTcpSocket>
#include <QSettings>
#include <QTimer>
#include <QTextStream>
#include <QByteArray>
#include <QQueue>
//My
#include "tlevelgauge.h"
#include "tconfig.h"

namespace LevelGauge
{

class TLS2 final
    : public TLevelGauge
{
    Q_OBJECT

public:
    explicit TLS2(QObject *parent = nullptr);
    ~TLS2();

public slots:
    void start() override;

private slots:
    void connentedSocket();
    void disconnentedSocket();
    void readyReadSocket();
    void errorOccurredSocket(QAbstractSocket::SocketError);
    void getData();
    void watchDocTimeout();

private:
    void parseAnswer(QByteArray& data);
    void parseTanksMeasument(const QByteArray& data);

    void skipLine(QTextStream& stream, const int count); //пропускает count строк из потока stream

    void upDateTanksMeasuments(); //отправляет набор команд для получение результатов измерений

    void sendCmd(const QByteArray &cmd);
    void sendNextCmd();
    void transferReset();

private:
    TConfig* _cnf = nullptr;
    QTcpSocket* _socket = nullptr;
    QTimer* _getDataTimer = nullptr;
    QTimer* _watchDoc = nullptr;

    TLevelGauge::TTanksMeasuments _tanksMeasuments; //очередь результатов измерений

    QByteArray _readBuffer; //буфер получения данных
    QQueue<QByteArray> _cmdQueue; //очередь команд
    int tick = 0; //номер такта
};

} //namespace LevelGauge

#endif // TLS2_H
