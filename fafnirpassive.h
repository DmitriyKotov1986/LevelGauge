#ifndef FAFNIRPASSIVE_H
#define FAFNIRPASSIVE_H

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

class FafnirPassive final
    : public TLevelGauge
{
    Q_OBJECT

public:
    explicit FafnirPassive(QObject *parent = nullptr);
    ~FafnirPassive();

public slots:
    void start() override;

private slots:
    void connentedSocket();
    void readyReadSocket();
    void disconnentedSocket();
    void errorOccurredSocket(QAbstractSocket::SocketError);
    void getData();

private:
    void connectToLevelGauge();

    void parseAnswer(const QByteArray& data);
    void parseTanksMeasument(const QByteArray& data);


    void sendCmd(const QByteArray &cmd);
    void sendNextCmd();
    void transferReset();

    float parseFloatValue(QTextStream &textStream, bool* ok = nullptr);

private:
    TConfig* _cnf = nullptr;
    QTcpSocket* _socket = nullptr;
    QTimer* _getDataTimer = nullptr;

    TLevelGauge::TTanksMeasuments _tanksMeasuments; //очередь результатов измерений

    QByteArray _readBuffer; //буфер получения данныъ

};

} //namespace LevelGauge

#endif // FAFNIRPASSIVE_H
