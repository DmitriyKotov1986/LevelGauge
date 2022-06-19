#ifndef TLS2_H
#define TLS2_H

#include <QObject>
#include <QtNetwork/QTcpSocket>
#include <QSettings>

#include "tlevelgauge.h"
#include "tconfig.h"

namespace LevelGauge {

class TLS2 final : public TLevelGauge
{
    Q_OBJECT

public:
    explicit TLS2(TConfig* cnf, QObject *parent = nullptr);
    ~TLS2() {};

public slots:
    void start() override {};

private:
    QTcpSocket* _socket;

 /*   void ParseAnswer(QByteArray &data);
    void ParseTanksMeasument(const QByteArray & data);
    void ParseTanksEnabled(const QByteArray &data);
    void ParseTanksDiametr(const QByteArray &data);
    void ParseTanksVolume(const QByteArray &data);
    void ParseTanksTilt(const QByteArray &data);
    void ParseTanksTCCoef(const QByteArray &data);
    void ParseTanksOffset(const QByteArray &data);

*/
};


/*
 * #include <QObject>
#include <QQueue>
#include <QVector>
#include <QDateTime>
#include <QtNetwork/QTcpSocket>
#include <QSettings>
#include <QByteArray>
#include <QTimer>

class TLevelGauge : public QTcpSocket
{

public:
    explicit TLevelGauge(QSettings &Config, QObject *parent = nullptr);
    ~TLevelGauge();

    TLevelGaugeData LevelGaugeData;

    void SendCmd(const QByteArray &cmd);
    void UpDateTanksConfig();
    void UpDateTanksMeasument();

    TLevelGaugeData &GetTanksInfo();

private:

    QByteArray ReadBuffer;
    QQueue<QByteArray> CmdQueue;
    bool Transfering = false; //флаг передачи данных
    QString LevelGaugeAdr;
    uint16_t LevelGaugePort;
    quint8 TLSModel = 2;
    QTimer WaitAnswerTimer;
    uint TransferAttempt = 0;

    void SendNextCmd();
    void TransferReset();
    void ParseAnswer(QByteArray &data);
    void ParseTanksMeasument(const QByteArray & data);
    void ParseTanksEnabled(const QByteArray &data);
    void ParseTanksDiametr(const QByteArray &data);
    void ParseTanksVolume(const QByteArray &data);
    void ParseTanksTilt(const QByteArray &data);
    void ParseTanksTCCoef(const QByteArray &data);
    void ParseTanksOffset(const QByteArray &data);

private slots:
    void onConnected();
    void onDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError Err);
    void onReadyRead();
    void onTransferTimeout();

signals:
    void GetDataComplite();
    void SendLogMsg(uint16_t Category, const QString &Msg);
};
*/

} //namespace LevelGauge

#endif // TLS2_H
