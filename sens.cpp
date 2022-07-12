#include "sens.h"

//Qt
#include <QCoreApplication>
//My
#include "common.h"

using namespace LevelGauge;

Sens::Sens(LevelGauge::TConfig* cnf, QObject* parent) :
    TLevelGauge(parent),
    _cnf(cnf)
{
    Q_ASSERT(cnf != nullptr);
}

Sens::~Sens()
{
}

void Sens::start()
{
    _getDataTimer = new QTimer();

    QObject::connect(_getDataTimer, SIGNAL(timeout()), SLOT(getData()));

    _getDataTimer->start(_cnf->sys_Interval());
}

void Sens::getData()
{

}
