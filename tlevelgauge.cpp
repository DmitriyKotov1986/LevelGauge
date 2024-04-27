#include "tlevelgauge.h"

using namespace LevelGauge;

TLevelGauge::TLevelGauge(QObject *parent)
    : QObject{parent}
{
    qRegisterMetaType<LevelGauge::TLevelGauge::TTanksMeasuments>("TTanksMeasuments");
    qRegisterMetaType<LevelGauge::TLevelGauge::TTanksConfigs>("TTanksConfigs");
}

bool TLevelGauge::checkMeasument(quint8 tankNumber, const TTankMeasument& measument)
{
    bool res = true;
    //проверям полученные значения
    if ((tankNumber < 1) || (tankNumber > MAX_TANK_NUMBER))
    {
        emit errorOccurred(QString("Invalid tank number. Number:%1. Tank ignored").arg(tankNumber));
        res = false;
    }
    if ((measument.volume < MIN_VOLUME) || (measument.volume > MAX_VOLUME))
    {
        emit errorOccurred(QString("Tank:%1 Invalid value received. Volume:%2. Tank ignored").arg(tankNumber).arg(measument.volume));
        res = false;
    }
    if ((measument.mass) < MIN_MASS || (measument.mass > MAX_MASS))
    {
        emit errorOccurred(QString("Tank:%1 Invalid value received. Mass:%2. Tank ignored").arg(tankNumber).arg(measument.mass));
        res = false;
    }
    if ((measument.density < MIN_DENSITY) || (measument.density > MAX_DENSITY))
    {
        emit errorOccurred(QString("Tank:%1 Invalid value received. Density:%2. Tank ignored").arg(tankNumber).arg(measument.density));
        res = false;
    }
    if ((measument.height < MIN_HEIGHT) || (measument.height > MAX_HEIGHT))
    {
        emit errorOccurred(QString("Tank:%1 Invalid value received. Height:%2. Tank ignored").arg(tankNumber).arg(measument.height));
        res = false;
    }
    if ((measument.temp < MIN_TEMP)||(measument.temp > MAX_TEMP))
    {
        emit errorOccurred(QString("Tank:%1 Invalid value received. Temp:%2. Tank ignored").arg(tankNumber).arg(measument.temp));
        res = false;
    }

    return res;
}
