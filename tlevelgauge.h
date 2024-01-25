#ifndef TLEVELGAUGE_H
#define TLEVELGAUGE_H

//STL
#include <exception>

//Qt
#include <QObject>
#include <QDateTime>
#include <QHash>
#include <QString>

//My
#include "Common/common.h"
#include "tconfig.h"

namespace LevelGauge
{

static const QDateTime DEFAULT_DATETIME = QDateTime::fromString("2000-01-01 00:00:00.001", Common::DATETIME_FORMAT);

class TLevelGauge : public QObject
{
    Q_OBJECT

public:
    struct TTankConfig
    {
        QDateTime dateTime = DEFAULT_DATETIME; //время получения данных
        bool enabled = false;
        qint16 diametr = -1; //диаметр
        qint32 volume = -1; //объем
        float tilt = 360.0; //наклон
        float TCCoef = -999.0;
        qint16 offset = 10000;
        QString product = "na";
    };

    struct  TTankMeasument
    {
        QDateTime dateTime = DEFAULT_DATETIME; //время получения данных
        qint32 volume = -1; //текущий объем
        qint32 mass = -1; //текущая масса
        float density = -1.0; //теккущая плотность
        float TKCorrect = -1000.0; //температураня коррекция плотности
        qint16 height = -1; //текущий уровень
        float water = -1.0; //текущий уровень воды
        float temp = -273.0; //текущая температура
    };

    typedef QHash<quint8, TTankConfig> TTanksConfigs;
    typedef QHash<quint8, TTankMeasument> TTanksMeasuments;

protected:
    constexpr static const quint8 MAX_TANK_NUMBER = 64;
    constexpr static const float MIN_TEMP = -273.0f;
    constexpr static const float MAX_TEMP = 60.0f;
    constexpr static const qint32 MIN_MASS = 0;
    constexpr static const qint32 MAX_MASS = 10000000;
    constexpr static const qint32 MIN_VOLUME = 0;
    constexpr static const qint32 MAX_VOLUME = 10000000;
    constexpr static const float MIN_DENSITY = 300.0f;
    constexpr static const float MAX_DENSITY = 1200.0f;
    constexpr static const qint16 MIN_HEIGHT = 0;
    constexpr static const qint16 MAX_HEIGHT = 32000;

    class TParseException
            : public std::runtime_error
    {
    public:
         TParseException(const QString& msg)
             : std::runtime_error(msg.toStdString())
         {
         }
    };

public:
    explicit TLevelGauge(QObject* parent = nullptr);
    virtual ~TLevelGauge() {};

protected:
    bool checkMeasument(quint8 tankNumber, const TTankMeasument& measument);

public slots:
    virtual void start() = 0;

signals:
    void getTanksMeasument(const LevelGauge::TLevelGauge::TTanksMeasuments& tanksMeasument);
    void getTanksConfig(const LevelGauge::TLevelGauge::TTanksConfigs& tankConfig);
    void errorOccurred(const QString& msg);
};

} //namespace LevelGauge

//декларируем типы для работы сигналах/слотах
Q_DECLARE_METATYPE(LevelGauge::TLevelGauge::TTanksMeasuments);
Q_DECLARE_METATYPE(LevelGauge::TLevelGauge::TTanksConfigs);

#endif // TLEVELGAUGE_H
