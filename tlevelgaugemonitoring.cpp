//Qt
#include <QDebug>
#include <QSqlError>
#include <QSqlQuery>
#include <QXmlStreamWriter>
#include <QCoreApplication>
#include <QFile>
#include <QThread>

//My
#include "tls2.h"
#include "tls4.h"
#include "sens.h"
#include "senspassive.h"
#include "fafnir.h"
#include "Common/common.h"

#include "tlevelgaugemonitoring.h"

using namespace LevelGauge;
using namespace Common;

TLevelGaugeMonitoring::TLevelGaugeMonitoring(TConfig* cnf, QObject* parent /* = nullptr*/)
    : QObject(parent)
    , _cnf(cnf)
{
    Q_ASSERT( _cnf != nullptr);

    //настраиваем подключениек БД
    _db = QSqlDatabase::addDatabase(_cnf->db_Driver(), "MainDB");
    _db.setDatabaseName(_cnf->db_DBName());
    _db.setUserName(_cnf->db_UserName());
    _db.setPassword(_cnf->db_Password());
    _db.setConnectOptions(_cnf->db_ConnectOptions());
    _db.setPort(_cnf->db_Port());
    _db.setHostName(_cnf->db_Host());

    //настраиваем подключение БД логирования
    auto logdb = QSqlDatabase::addDatabase(_cnf->db_Driver(), "LoglogDB");
    logdb.setDatabaseName(_cnf->db_DBName());
    logdb.setUserName(_cnf->db_UserName());
    logdb.setPassword(_cnf->db_Password());
    logdb.setConnectOptions(_cnf->db_ConnectOptions());
    logdb.setPort(_cnf->db_Port());
    logdb.setHostName(_cnf->db_Host());

    _loger = Common::TDBLoger::DBLoger(&logdb, _cnf->sys_DebugMode());
    if (_loger->isError())
    {
        QString msg = QString("Loger initialization error. Error: %1").arg(_loger->errorString());
        qCritical() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
        Common::writeLogFile("ERR>", msg);

        exit(EXIT_CODE::START_LOGGER_ERR); // -1
    }

    //создаем поток обработки HTTP Запросов
    QString url = QString("http://%1:%2/CGI/LEVELGAUGE&%3&%4").arg(_cnf->srv_Host()).arg(_cnf->srv_Port()).
                    arg(_cnf->srv_UserName()).arg(_cnf->srv_Password());
    _HTTPQuery = THTTPQuery::HTTPQuery(url, nullptr);

    QThread* HTTPQueryThread = new QThread(this);
    _HTTPQuery->moveToThread(HTTPQueryThread);

    QObject::connect(this, SIGNAL(sendHTTP(const QByteArray&)), _HTTPQuery, SLOT(send(const QByteArray&)));
    QObject::connect(_HTTPQuery, SIGNAL(getAnswer(const QByteArray&)), SLOT(getAnswerHTTP(const QByteArray&)));
    QObject::connect(_HTTPQuery, SIGNAL(errorOccurred(const QString&)), SLOT(errorOccurredHTTP(const QString&)));

    QObject::connect(this, SIGNAL(finished()), HTTPQueryThread, SLOT(quit())); //сигнал на завершение
    QObject::connect(HTTPQueryThread, SIGNAL(finished()), HTTPQueryThread, SLOT(deleteLater())); //уничтожиться после остановки

    HTTPQueryThread->start();

   //подключаем уровнемер
    _levelGauge = loadLG();
    QThread* levelGaugeThread = new QThread(this);
    _levelGauge->moveToThread(levelGaugeThread);

    QObject::connect(this, SIGNAL(startLG()), _levelGauge, SLOT(start()));
    QObject::connect(_levelGauge, SIGNAL(getTanksMeasument(const TLevelGauge::TTanksMeasuments&)),
                     SLOT(getTanksMeasument(const TLevelGauge::TTanksMeasuments&)));
    QObject::connect(_levelGauge, SIGNAL(getTanksConfig(const TLevelGauge::TTanksConfigs&)),
                     SLOT(getTanksConfig(const TLevelGauge::TTanksConfigs&)));
    QObject::connect(_levelGauge, SIGNAL(errorOccurred(const QString&)), SLOT(errorOccurredLG(const QString&)));

    QObject::connect(this, SIGNAL(finished()), levelGaugeThread, SLOT(quit())); //сигнал на завершение
    QObject::connect(levelGaugeThread, SIGNAL(finished()), levelGaugeThread, SLOT(deleteLater())); //уничтожиться после остановки

    levelGaugeThread->start();

    //таймер отправки HTTP запроса
    _sendHTTPTimer = new QTimer();
    _sendHTTPTimer->setInterval(_cnf->sys_Interval());

    QObject::connect(_sendHTTPTimer, SIGNAL(timeout()), SLOT(sendToHTTPServer()));

    //инициализируем пременные
    //_maxTanksMasumentsID = _cnf->srv_LastTankMeasumentID();
    //_maxTanksConfigsID = _cnf->srv_LastTankConfigID();
}

TLevelGaugeMonitoring::~TLevelGaugeMonitoring()
{
    if (_levelGauge != nullptr)
    {
        //ничего не делаем. остановится  по сигналу finished
    }

    if (_sendHTTPTimer != nullptr)
    {
        _sendHTTPTimer->stop();
        _sendHTTPTimer->deleteLater();
    }

    if (_db.isOpen())
    {
        _db.close();
    }
    _loger->sendLogMsg(TDBLoger::MSG_CODE::OK_CODE, "Successfully finished");

    if (_loger != nullptr)
    {
        TDBLoger::deleteDBLoger();
    }

    if (_HTTPQuery != nullptr)
    {
        THTTPQuery::deleteTHTTPQuery();
    }
}

void TLevelGaugeMonitoring::start()
{
    Q_CHECK_PTR(_HTTPQuery);
    Q_CHECK_PTR(_levelGauge);
    Q_CHECK_PTR(_sendHTTPTimer);

    //подключаемся к БД
    if (!_db.open())
    {
        _loger->sendLogMsg(TDBLoger::MSG_CODE::CRITICAL_CODE, QString("Cannot connect to database. Error: %1").arg(_db.lastError().text()));

        emit finished();

        exit(EXIT_CODE::SQL_NOT_OPEN_DB);
    };


    //запускаем опрос уровнемеров
    emit startLG();

    //запукаем таймер отправки HTTP запроов
    _sendHTTPTimer->start();

     _loger->sendLogMsg(TDBLoger::MSG_CODE::OK_CODE, "Successfully started");
}

TLevelGauge* TLevelGaugeMonitoring::loadLG()
{
    switch (_cnf->lg_TLS())
    {
    case 2: return new LevelGauge::TLS2();
    case 4: return new LevelGauge::TLS4();
    case 100: return new LevelGauge::Sens();
    case 101: return new LevelGauge::SensPassive();
    case 200: return new LevelGauge::Fafnir();
    default:
    {
        QString msg = "Undefine type of level gauge. Check blok [LEVELGAUGE]/'TLS' in config file";
        saveLogToFile(msg);

        //тут еще цикл обработки событий не запущен - поэтому просто выходим
        exit(EXIT_CODE::LEVELGAUGE_UNDEFINE_TYPE);
    }
    }

    return nullptr;
}

void TLevelGaugeMonitoring::saveTanksMasumentToDB(const TLevelGauge::TTanksMeasuments& tanksMeasument)
{
    Q_CHECK_PTR(_levelGauge);
    Q_CHECK_PTR(_loger);

    Q_ASSERT(!tanksMeasument.isEmpty());

    if (tanksMeasument.isEmpty())
    {
        _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, "No data about measuments from the level gauge");
        return; //если нечего записывать - выходим
    }


    for(auto tanksMeasument_it = tanksMeasument.begin(); tanksMeasument_it != tanksMeasument.end(); ++tanksMeasument_it)
    {
        //firebird not support multirow insert
        QString queryText = "INSERT INTO TANKSMEASUMENTS (TANK_NUMBER, DATE_TIME, VOLUME, MASS, DENSITY, TCCORRECT, HEIGHT, WATER, TEMP) "
                            " VALUES ("
                            + QString::number(tanksMeasument_it.key()) + ", " +
                            "'" + tanksMeasument_it->dateTime.toString("yyyy-MM-dd hh:mm:ss.zzz") + "', " +
                            QString::number(tanksMeasument_it->volume, 'f', 0) + ", " +
                            QString::number(tanksMeasument_it->mass, 'f', 0) + ", " +
                            QString::number(tanksMeasument_it->density, 'f', 1) + ", " +
                            QString::number(tanksMeasument_it->TKCorrect, 'f', 2) + ", " +
                            QString::number(tanksMeasument_it->height, 'f', 0) + ", " +
                            QString::number(tanksMeasument_it->water, 'f', 1) + ", " +
                            QString::number(tanksMeasument_it->temp, 'f', 1) + ")";

        DBQueryExecute(_db, queryText);
    }  
}

void TLevelGaugeMonitoring::saveTanksConfigToDB(const TLevelGauge::TTanksConfigs& tanksConfig)
{
    Q_CHECK_PTR(_levelGauge);
    Q_CHECK_PTR(_loger);

    Q_ASSERT(!tanksConfig.isEmpty());

    if (tanksConfig.isEmpty())
    {
         _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, "No data about tanks config from the level gauge");

        return; //если нечего записывать - выходим
    }

    for (const auto& numberTank : tanksConfig.keys())
    {
        //firebird not support multirow insert
        TLevelGauge::TTankConfig tmp = tanksConfig[numberTank];
        QString queryText = "INSERT INTO TANKSCONFIG (TANK_NUMBER, DATE_TIME, ENABLED, DIAMETR, VOLUME, TILT, TCCOEF, OFFSET, PRODUCT) "
                            "VALUES ( " +
                            QString::number(numberTank) + ", " +
                            "'" + tmp.dateTime.toString("yyyy-MM-dd hh:mm:ss.zzz") + "', " +
                            QString::number(tmp.enabled) + ", " +
                            QString::number(tmp.diametr, 'f', 0) + ", " +
                            QString::number(tmp.volume, 'f', 0) + ", " +
                            QString::number(tmp.tilt, 'f', 1) + ", " +
                            QString::number(tmp.TCCoef, 'f', 2) + ", " +
                            QString::number(tmp.offset, 'f', 0) + ", " +
                            "'" + tmp.product + "')";

        DBQueryExecute(_db, queryText);
    } 
}

void TLevelGaugeMonitoring::getTanksMeasument(const TLevelGauge::TTanksMeasuments& tanksMeasument)
{
    Q_CHECK_PTR(_loger);

    saveTanksMasumentToDB(tanksMeasument);

    _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("Data about measuments has been successfully received from the level gauge. "
                                           "Tanks count: %1").arg(tanksMeasument.size()));
}

void TLevelGaugeMonitoring::getTanksConfig(const TLevelGauge::TTanksConfigs& tanksConfig)
{
    Q_CHECK_PTR(_loger);

    saveTanksConfigToDB(tanksConfig);

    _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("Data about tanks configuration has been successfully received from the level gauge. "
                                                   "Tanks count: %1").arg(tanksConfig.size()));
}

void LevelGauge::TLevelGaugeMonitoring::errorOccurredLG(const QString& msg)
{
    Q_CHECK_PTR(_loger);

    _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("Error getting data from level gauge Msg: " + msg));
}


void TLevelGaugeMonitoring::sendToHTTPServer()
{
    Q_CHECK_PTR(_loger);

    //если ответ на предыдущий запрос еще не получен - то пропускаем этот такт таймера
    if (_sending) {
        return;
    }

    _sendingTanksConfigsID.clear();
    _sendingTanksMasumentsID.clear();

    _db.transaction();
    QSqlQuery query(_db);

    QString queryText = "SELECT FIRST " + QString::number(_cnf->srv_MaxRecord()) + " " +
                       "ID, DATE_TIME, TANK_NUMBER, ENABLED, DIAMETR, VOLUME, TILT, TCCOEF, OFFSET, PRODUCT "
                       "FROM TANKSCONFIG "   
                       "ORDER BY ID";


    if (!query.exec(queryText))
    {
       errorDBQuery(_db, query);
    }

    //форматируем XML докумен
    QString XMLStr;
    QXmlStreamWriter XMLWriter(&XMLStr);
    XMLWriter.setAutoFormatting(true);
    XMLWriter.writeStartDocument("1.0");
    XMLWriter.writeStartElement("Root");
    XMLWriter.writeTextElement("AZSCode", _cnf->srv_UserName());
    XMLWriter.writeTextElement("ClientVersion", QCoreApplication::applicationVersion());

    while (query.next())
    {
       // qDebug() << Query.value("TANK_NUMBER").toString();
        XMLWriter.writeStartElement("LevelGaugeConfig");
        XMLWriter.writeTextElement("DateTime", query.value("DATE_TIME").toDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"));
        XMLWriter.writeTextElement("TankNumber", query.value("TANK_NUMBER").toString());
        XMLWriter.writeTextElement("Enabled", query.value("ENABLED").toString());
        XMLWriter.writeTextElement("Diametr", query.value("DIAMETR").toString());
        XMLWriter.writeTextElement("Volume", query.value("VOLUME").toString());
        XMLWriter.writeTextElement("Tilt", query.value("TILT").toString());
        XMLWriter.writeTextElement("TCCoef", query.value("TCCOEF").toString());
        XMLWriter.writeTextElement("Offset", query.value("OFFSET").toString());
        XMLWriter.writeTextElement("Product", query.value("PRODUCT").toString());
        XMLWriter.writeEndElement();

        _sendingTanksConfigsID.push_back("'" + query.value("ID").toString() + "'");
    }

    queryText = "SELECT FIRST " + QString::number(_cnf->srv_MaxRecord()) + " " +
                "ID, TANK_NUMBER, DATE_TIME, VOLUME, MASS, DENSITY, TCCORRECT, HEIGHT, WATER, TEMP "
                "FROM TANKSMEASUMENTS "
                "ORDER BY ID";

    if (!query.exec(queryText))
    {
        errorDBQuery(_db, query);
    }

    while (query.next())
    {
        XMLWriter.writeStartElement("LevelGaugeMeasument");
        XMLWriter.writeTextElement("DateTime", query.value("DATE_TIME").toDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz"));
        XMLWriter.writeTextElement("TankNumber", query.value("TANK_NUMBER").toString());
        XMLWriter.writeTextElement("Volume", query.value("VOLUME").toString());
        XMLWriter.writeTextElement("Mass", query.value("MASS").toString());
        XMLWriter.writeTextElement("Density", query.value("DENSITY").toString());
        XMLWriter.writeTextElement("TCCorrect", query.value("TCCORRECT").toString());
        XMLWriter.writeTextElement("Height", query.value("HEIGHT").toString());
        XMLWriter.writeTextElement("Water", query.value("WATER").toString());
        XMLWriter.writeTextElement("Temp", query.value("TEMP").toString());
        XMLWriter.writeEndElement();

        _sendingTanksMasumentsID.push_back(QString("'%1'").arg(query.value("ID").toString()));
    }

    XMLWriter.writeEndElement(); //root
    XMLWriter.writeEndDocument();

    DBCommit(_db);

    //отправляем запрос
    _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("Sending a request. Size: %1 byte").arg(XMLStr.toUtf8().size()));

    _sending = true; //устанавливаем флаг

    emit sendHTTP(XMLStr.toUtf8());
}

void TLevelGaugeMonitoring::errorOccurredHTTP(const QString& msg)
{
    Q_CHECK_PTR(_loger);

    _sending = false;
    //очищаем очереди
    _sendingTanksMasumentsID.clear();
    _sendingTanksConfigsID.clear();

    _loger->sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, "Error sending data to HTTP server. Msg: " + msg);
}

void TLevelGaugeMonitoring::getAnswerHTTP(const QByteArray &answer)
{
    Q_CHECK_PTR(_loger);

    _sending = false;

    if (answer.left(2) == "OK") {
        if (!_sendingTanksMasumentsID.isEmpty()) {
            QString queryText = "DELETE FROM TANKSMEASUMENTS "
                                "WHERE ID IN (" + _sendingTanksMasumentsID.join(",") + ")";

            DBQueryExecute(_db, queryText);
        }
        if (!_sendingTanksConfigsID.empty()) {
            QString queryText = "DELETE FROM TANKSCONFIG "
                                "WHERE ID IN (" + _sendingTanksConfigsID.join(",") + ")";

            DBQueryExecute(_db, queryText);
        }

        //если на удаление было записей _cnf->srv_MaxRecord() значит скорее всего в БД есть еще записи
        //поэтому повторяем отправку данных
        bool neetSending = (_sendingTanksMasumentsID.size() == _cnf->srv_MaxRecord()) || (_sendingTanksConfigsID.size() == _cnf->srv_MaxRecord());

        _loger->sendLogMsg(TDBLoger::MSG_CODE::INFORMATION_CODE, QString("Data has been successfully sent to the server"));

        if (neetSending)
        {
            _loger->sendLogMsg(TDBLoger::INFORMATION_CODE, QString("Not all data were sent to the server. A repeat request will be sent."));

            sendToHTTPServer(); //если есть еще данные для отправки - повторяем отправку данных
        }
   }
   else {
       _loger->sendLogMsg(TDBLoger::MSG_CODE::ERROR_CODE, QString("Failed to send data to the server. Server answer: %1").arg(answer));
   }
}
