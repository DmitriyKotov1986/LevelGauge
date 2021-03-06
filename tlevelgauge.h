#ifndef TLEVELGAUGE_H
#define TLEVELGAUGE_H

//Qt
#include <QObject>
#include <QDateTime>
#include <QHash>
#include <QString>

//My
#include "tconfig.h"

namespace LevelGauge {

static const QDateTime DEFAULT_DATETIME = QDateTime::fromString("2000-01-01 00:00:00.001", "yyyy-MM-dd hh:mm:ss.zzz");

class TLevelGauge : public QObject
{
    Q_OBJECT

public:
    typedef struct {
        QDateTime dateTime = DEFAULT_DATETIME; //время получения данных
        bool enabled = false;
        qint16 diametr = -1; //диаметр
        qint32 volume = -1; //объем
        float tilt = 360.0; //наклон
        float TCCoef = -999.0;
        qint16 offset = 10000;
        QString product = "na";
    }  TTankConfig;

    typedef struct {
        QDateTime dateTime = DEFAULT_DATETIME; //время получения данных
        qint32 volume = -1; //текущий объем
        qint32 mass = -1; //текущая масса
        float density = -1.0; //теккущая плотность
        float TKCorrect = -1000.0; //температураня коррекция плотности
        qint16 height = -1; //текущий уровеньb
        float water = -1.0; //текущий уровень воды
        float temp = -273.0; //текущая температура
    } TTankMeasument;

    typedef QHash<quint8, TTankConfig> TTanksConfigs;
    typedef QHash<quint8, TTankMeasument> TTanksMeasuments;

public:
    explicit TLevelGauge(QObject* parent = nullptr) : QObject(parent) {};
    virtual ~TLevelGauge() {};

public slots:
    virtual void start() = 0;

signals:
    void getTanksMeasument(const TLevelGauge::TTanksMeasuments& tanksMeasument);
    void getTanksConfig(const TLevelGauge::TTanksConfigs& tankConfig);
    void errorOccurred(const QString& msg);
};

} //namespace LevelGauge

//декларируем типы для работы сигналах/слотах
Q_DECLARE_METATYPE(LevelGauge::TLevelGauge::TTanksMeasuments);
Q_DECLARE_METATYPE(LevelGauge::TLevelGauge::TTanksConfigs);

#endif // TLEVELGAUGE_H
