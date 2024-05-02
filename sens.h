//реализует активный режим протокола СЕНС
#ifndef SENS_H
#define SENS_H

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

namespace LevelGauge {

class Sens final : public TLevelGauge
{
    Q_OBJECT

public:
    explicit Sens(QObject *parent = nullptr);
    ~Sens();

public slots:
    void start() override;

private slots:
    void connentedSocket();
    void disconnentedSocket();
    void readyReadSocket();
    void errorOccurredSocket(QAbstractSocket::SocketError);
    void getData();
    void watchDocTimeout();
    void sendData(); //отправляет сигналы с готовыми данными

private:
    void sendCmd(const QByteArray &cmd);
    void sendNextCmd();
    void transferReset();

    void upDataTanksMeasuments(); //отправляет набор команд для получение результатов измерений

    unsigned char CRC(const QByteArray& cmd);
    float float24ToFloat32(const QByteArray& number);

    void parseAnswer(QByteArray data);
    TLevelGauge::TTankMeasument parseTankMeasument(QDataStream& dataStream);

private:
    TConfig* _cnf = nullptr;
    QTcpSocket* _socket = nullptr;
    QTimer* _getDataTimer = nullptr;
    QTimer* _watchDoc = nullptr;
    QTimer* _sendDataTimer = nullptr;

    TLevelGauge::TTanksMeasuments _tanksMeasuments; //очередь результатов измерений

    QByteArray _readBuffer; //буфер получения данныъ
    QQueue<QByteArray> _cmdQueue; //очередь команд

    int tick = 0; //номер такта
};

} //namespace LevelGauge

#endif // SENS_H
