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
#include <QModbusTcpClient>
//My
#include "tlevelgauge.h"
#include "tconfig.h"

namespace LevelGauge {

class Sens final : public TLevelGauge
{
    Q_OBJECT

public:
    explicit Sens(LevelGauge::TConfig* cnf, QObject *parent = nullptr);
    ~Sens();

public slots:
    void start() override;

private slots:
    void getData();

private:
    TConfig* _cnf = nullptr;
    QTimer* _getDataTimer = nullptr;
    QModbusTcpClient* _modbusTTcpClient = nullptr;

    TLevelGauge::TTanksConfigs _tanksConfigs; //очередь конфигураций резервуаров
    TLevelGauge::TTanksMeasuments _tanksMeasuments; //очередь результатов измерений
};

} //namespace LevelGauge

#endif // TLS2_H
