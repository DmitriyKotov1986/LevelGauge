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

namespace LevelGauge
{

class TLS4 final
    : public TLevelGauge
{
    Q_OBJECT

public:
    explicit TLS4(QObject *parent = nullptr);
    ~TLS4();

public slots:
    void start() override;

private slots:
    void connentedSocket();
    void readyReadSocket();
    void disconnentedSocket();
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

    float parseFloatValue(QTextStream &textStream, quint8 tankNumber, const QString& parametrName);
    qint64 parseIntValue(QTextStream &textStream, quint8 tankNumber, const QString& parametrName);

private:
    TConfig* _cnf = nullptr;
    QTcpSocket* _socket = nullptr;
    QTimer* _getDataTimer = nullptr;
    QTimer* _watchDoc = nullptr;

    TLevelGauge::TTanksMeasuments _tanksMeasuments; //очередь результатов измерений

    QByteArray _readBuffer; //буфер получения данныъ
    QQueue<QByteArray> _cmdQueue; //очередь команд
    int tick = 0; //номер такта
};

} //namespace LevelGauge

#endif // TLS4_H
