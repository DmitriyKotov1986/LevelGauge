//Qt
#include <QCoreApplication>
#include <QTimer>
#include <QCommandLineParser>
#include <windows.h>

//My
#include "tconfig.h"
#include "tlevelgaugemonitoring.h"
#include "Common/common.h"
#include "Common/regcheck.h"

using namespace LevelGauge;

using namespace Common;

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    //устаналиваем основные настройки
    QCoreApplication::setApplicationName("LevelGauge");
    QCoreApplication::setOrganizationName("OOO 'SA'");
    QCoreApplication::setApplicationVersion(QString("Version:0.3 Build: %1 %2").arg(__DATE__).arg(__TIME__));

    setlocale(LC_CTYPE, ""); //настраиваем локаль

    //Создаем парсер параметров командной строки
    QCommandLineParser parser;
    parser.setApplicationDescription("Program for receiving data from the level gauge and sending and sending them to the HTTP server.");
    parser.addHelpOption();
    parser.addVersionOption();

    //добавляем опцию Config
    QCommandLineOption Config(QStringList() << "Config", "Config file name", "ConfigFileNameValue", "LevelGauge.ini");
    parser.addOption(Config);

    //Парсим опции командной строки
    parser.process(a);

    //читаем конфигурацию из файла
    QString configFileName = parser.value(Config);
    if (!parser.isSet(Config)) {
        configFileName = a.applicationDirPath() +"/" + parser.value(Config);
    }

    //Читаем конигурацию
    TConfig* cnf = TConfig::config(configFileName);
    if (cnf->isError()) {
        QString msg = "Error load configuration: " + cnf->errorString();
        qCritical() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
        Common::writeLogFile("ERR>", msg);

        return EXIT_CODE::LOAD_CONFIG_ERR; // -1
    }

    //проверяем регистрацию ПО
    RegCheck* regCheck = new RegCheck(cnf->srv_UserName());

    if (regCheck->isChecket())
    {
        QString msg = QString("Registration verification passed successfully");
        qDebug() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
    }
    else
    {
        QString msg = QString("Unregistered copy of the program. Registration key: %1. Message: %2")
                .arg(regCheck->id()).arg(regCheck->messageString());

        qCritical() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
        Common::writeLogFile("ERR>", msg);

        TConfig::deleteConfig();
        delete regCheck;

        return EXIT_CODE::UNREGISTER_COPY; // -4
    }

    delete regCheck;

    //настраиваем таймер
    QTimer startTimer;
    startTimer.setInterval(0);       //таймер сработает так быстро как только это возможно
    startTimer.setSingleShot(true);  //таймер сработает 1 раз

    //создаем основной класс программы
    TLevelGaugeMonitoring levelGaugeMonitoring(cnf, &a);

    //При запуске выполняем слот Start
    QObject::connect(&startTimer, SIGNAL(timeout()), &levelGaugeMonitoring, SLOT(start()));
    //Для завершения работы необходимоподать сигнал Finished
    QObject::connect(&levelGaugeMonitoring, SIGNAL(finished()), &a, SLOT(quit()));

    //запускаем таймер
    startTimer.start();
    //запускаем цикл обработчика событий
    auto res =  a.exec();

    TConfig::deleteConfig();

    return res;
}
