#include "tlevelgauge.h"

using namespace LevelGauge;

bool TLevelGauge::checkMeasument(quint8 tankNumber, const TTankMeasument& measument)
{
    bool res = true;
    //проверям полученные значения
    if ((tankNumber < 1) || (tankNumber > 10))
    {
        emit errorOccurred(QString("Invalid tank number. Number:%1. Tank ignored.").arg(tankNumber));
        res = false;
    }
    if ((measument.volume < 10) || (measument.volume > 10000000))
    {
        emit errorOccurred(QString("Tank:%1 Invalid value received. Volume:%2. Tank ignored.").arg(tankNumber).arg(measument.volume));
        res = false;
    }
    if ((measument.mass) < 10 || (measument.mass > 10000000))
    {
        emit errorOccurred(QString("Tank:%1 Invalid value received. Mass:%2. Tank ignored.").arg(tankNumber).arg(measument.mass));
        res = false;
    }
    if ((measument.density < 400.0) || (measument.density > 1200.0))
    {
        emit errorOccurred(QString("Tank:%1 Invalid value received. Density:%2. Tank ignored.").arg(tankNumber).arg(measument.density));
        res = false;
    }
    if ((measument.height < 10) || (measument.height > 20000))
    {
        emit errorOccurred(QString("Tank:%1 Invalid value received. Height:%2. Tank ignored.").arg(tankNumber).arg(measument.height));
        res = false;
    }
    if ((measument.temp < -60.0)||(measument.temp > 60.0))
    {
        emit errorOccurred(QString("Tank:%1 Invalid value received. Temp:%2. Tank ignored.").arg(tankNumber).arg(measument.temp));
        res = false;
    }

    return res;
}
