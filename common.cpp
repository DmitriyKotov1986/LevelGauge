#include "common.h"

//Qt
#include <QCoreApplication>
#include <QFile>
#include <QDateTime>
//My
#include "tconfig.h"

using namespace LevelGauge;

void LevelGauge::writeLogFile(const QString& prefix, const QString& msg) {
    QFile file(QCoreApplication::applicationDirPath() + LOG_FILE_NAME);
    if (file.open(QFile::WriteOnly | QFile::Append | QFile::Text)) {
        QByteArray tmp = QString("%1 %2\n%3\n\n").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz")).arg(prefix).arg(msg).toUtf8();
        file.write(tmp);
        file.close();
    }
}

void LevelGauge::writeDebugLogFile(const QString& prefix, const QString& msg) {
    #ifdef QT_DEBUG
        writeLogFile(prefix, msg);
    #endif
}
