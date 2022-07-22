#ifndef SENSPASSIVE_H
#define SENSPASSIVE_H

//Qt
#include <QObject>
#include <QtNetwork/QTcpSocket>
#include <QSettings>
#include <QTimer>
#include <QTextStream>
#include <QByteArray>
#include <QMap>
#include <QTime>

//My
#include "tlevelgauge.h"
#include "tconfig.h"

namespace LevelGauge {

class SensPassive final : public TLevelGauge
{
    Q_OBJECT

public:
    explicit SensPassive(LevelGauge::TConfig* cnf, QObject *parent = nullptr);
    ~SensPassive();

public slots:
    void start() override;

private slots:
    void connentedSocket();
    void disconnentedSocket();
    void readyReadSocket();
    void errorOccurredSocket(QAbstractSocket::SocketError);
    void getData();
    void watchDocTimeout(); //таймайт ватчдога
    void sendData(); //отправляет сигналы с готовыми данными

private:
    void transferReset();

    unsigned char CRC(const QByteArray& cmd);
    float float24ToFloat32(const QByteArray& number);

    void parseAnswer(QByteArray data);
    TLevelGauge::TTankMeasument parseTankMeasument(QDataStream& dataStream);
    TLevelGauge::TTankConfig parseTankConfig(QDataStream& dataStream);

private:
    TConfig* _cnf = nullptr;
    QTcpSocket* _socket = nullptr;
    QTimer* _getDataTimer = nullptr;
    QTimer* _watchDoc = nullptr;
    QTimer* _sendDataTimer = nullptr;

    TLevelGauge::TTanksConfigs _tanksConfigs; //очередь конфигураций резервуаров
    TLevelGauge::TTanksMeasuments _tanksMeasuments; //очередь результатов измерений

    QMap<uint8_t, QTime> _lastGetMeausumentsData; //ключ - адрес резервуара, значение время последнего получения данных о измерениях
    QMap<uint8_t, QTime> _lastGetConfigsData; //ключ - адрес резервуара, значение время последнего получения данных конфигурации
};

} //namespace LevelGauge

#endif // SENSPASSIVE_H
