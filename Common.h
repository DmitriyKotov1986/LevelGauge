#ifndef COMMON_H
#define COMMON_H

//Qt
#include <QString>

namespace LevelGauge {

void writeLogFile(const QString& prefix, const QString& msg);
void writeDebugLogFile(const QString& prefix, const QString& msg);

}//LevelGauge

#endif // COMMON_H
