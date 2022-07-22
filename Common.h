#ifndef COMMON_H
#define COMMON_H

//Qt
#include <QString>
#include <QFile>
#include <QDateTime>
#include <QDebug>
#include <QFileInfo>

enum EXIT_CODE: int {
    //SQL
    OK = 0,
    LOAD_CONFIG_ERR = -1,
    SQL_EXECUTE_QUERY_ERR = -2,
    SQL_COMMIT_ERR = -3,
    //LevelGauge
    LEVELGAUGE_UNDEFINE_TYPE = -100
};

static const QString LOG_FILE_NAME = "./Log/LevelGauge.log"; //имя файла лога

static void writeLogFile(const QString& prefix, const QString& msg) {
    QFile file(LOG_FILE_NAME);
    if (file.open(QFile::WriteOnly | QFile::Append | QFile::Text)) {
        QByteArray tmp = QString("%1 %2\n%3\n\n").arg(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")).arg(prefix).arg(msg).toUtf8();
        file.write(tmp);
        file.close();
    }
    else {
        qCritical() << QString("%1(!)Message not save to log file: %2. Message: %3 %4").arg(QDateTime::currentDateTime().toString("hh:mm:ss"))
                       .arg(QFileInfo(LOG_FILE_NAME).absoluteFilePath()).arg(prefix).arg(msg);
    }
}

static void writeDebugLogFile(const QString& prefix, const QString& msg) {
    #ifdef QT_DEBUG
        writeLogFile(prefix, msg);
    #endif
}


#endif // COMMON_H
