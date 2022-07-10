#ifndef TLS4_H
#define TLS4_H

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

class TLS4 final : public TLevelGauge
{
    Q_OBJECT

public:
    explicit TLS4(TConfig* cnf, QObject *parent = nullptr);
    ~TLS4();

public slots:
    void start() override;

private slots:
    void connentedSocket();
    void readyReadSocket();
    void disconnentedSocket();
    void errorOccurredSocket(QAbstractSocket::SocketError);
    void getData();

private:
    void parseAnswer(QByteArray& data);
    void parseTanksMeasument(const QByteArray& data);
    void parseTanksEnabled(const QByteArray& data);
    void parseTanksDiametr(const QByteArray& data);
    void parseTanksVolume(const QByteArray& data);
    void parseTanksTilt(const QByteArray& data);
    void parseTanksTCCoef(const QByteArray& data);
    void parseTanksOffset(const QByteArray& data);

    void skipLine(QTextStream& stream, const int count); //пропускает count строк из потока stream

    void upDateTanksConfigs(); //отправляет набор команд ля получения конфигурации резервуаров
    void upDateTanksMeasuments(); //отправляет набор команд для получение результатов измерений

    void sendCmd(const QByteArray &cmd);
    void sendNextCmd();
    void transferReset();

private:
    TConfig* _cnf = nullptr;
    QTcpSocket* _socket = nullptr;
    QTimer* _getDataTimer = nullptr;

    TLevelGauge::TTanksConfigs _tanksConfigs; //очередь конфигураций резервуаров
    TLevelGauge::TTanksMeasuments _tanksMeasuments; //очередь результатов измерений

    QByteArray readBuffer; //буфер получения данныъ
    QQueue<QByteArray> cmdQueue; //очередь команд
    int tick = 0; //номер такта
};

} //namespace LevelGauge

#endif // TLS4_H
