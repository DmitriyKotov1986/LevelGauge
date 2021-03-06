#include "tlevelgaugemonitoring.h"

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
#include "common.h"

using namespace LevelGauge;

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
    _maxTanksMasumentsID = _cnf->srv_LastTankMeasumentID();
    _maxTanksConfigsID = _cnf->srv_LastTankConfigID();
}

TLevelGaugeMonitoring::~TLevelGaugeMonitoring()
{
    Q_ASSERT(_levelGauge != nullptr);
    Q_ASSERT(_sendHTTPTimer != nullptr);
    Q_ASSERT(_HTTPQuery != nullptr);

    sendLogMsg(MSG_CODE::CODE_OK, "Successfully finished");

    if (_levelGauge != nullptr) {
        //ничего не делаем. остановится  по сигналу finished
    }

    if (_sendHTTPTimer != nullptr) {
        _sendHTTPTimer->stop();
        _sendHTTPTimer->deleteLater();
    }


    if (_HTTPQuery != nullptr) {
        //ничего не делаем. остановится  по сигналу finished
    }

    if (_db.isOpen()) {
        _db.close();
    }
}

void TLevelGaugeMonitoring::start()
{
    Q_ASSERT(_HTTPQuery != nullptr);
    Q_ASSERT(_levelGauge != nullptr);
    Q_ASSERT(_sendHTTPTimer != nullptr);

    //подключаемся к БД
    if (!_db.open()) {
        QString msg = QString("Cannot connect to database. Error: %1").arg(_db.lastError().text());
        qCritical() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
        saveLogToFile(msg);

        emit finished();

        return;
    };

    //запускаем опрос уровнемеров
    emit startLG();

    //запукаем таймер отправки HTTP запроов
    _sendHTTPTimer->start();

    sendLogMsg(MSG_CODE::CODE_OK, "Successfully started");
}

TLevelGauge* TLevelGaugeMonitoring::loadLG()
{
    switch (_cnf->lg_TLS()) {
    case 2: return new LevelGauge::TLS2(_cnf);
    case 4: return new LevelGauge::TLS4(_cnf);
    case 100: return new LevelGauge::Sens(_cnf);
    case 101: return new LevelGauge::SensPassive(_cnf);
    default: {
        QString msg = "Undefine type of level gauge. Check blok [LEVELGAUGE]/'TLS' in config file";
        saveLogToFile(msg);
        //тут еще цикл обработки событий не запущен - поэтому просто выходим
        exit(EXIT_CODE::LEVELGAUGE_UNDEFINE_TYPE);
    }
    }
    return nullptr;
}


void TLevelGaugeMonitoring::sendLogMsg(uint16_t category, const QString& msg)
{
    if (_cnf->sys_DebugMode()) {
        qDebug() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
    }

    writeDebugLogFile("LOG>", msg);

    QString queryText = "INSERT INTO LOG (CATEGORY, SENDER, MSG) VALUES ( "
                        + QString::number(category) + ", "
                        "\'LevelGauge\', "
                        "\'" + msg +"\'"
                        ")";

    DBQueryExecute(queryText);
}

void TLevelGaugeMonitoring::saveLogToFile(const QString& msg)
{
    writeLogFile("LOG>", msg);
}

void TLevelGaugeMonitoring::saveTanksMasumentToDB(const TLevelGauge::TTanksMeasuments& tanksMeasument)
{
    Q_ASSERT(_levelGauge != nullptr);
    Q_ASSERT(!tanksMeasument.isEmpty());

    if (tanksMeasument.isEmpty()) {
        sendLogMsg(MSG_CODE::CODE_INFORMATION, "No data about measuments from the level gauge");
        return; //если нечего записывать - выходим
    }


    for (const auto& tankNumber : tanksMeasument.keys()) {
        //firebird not support multirow insert
        TLevelGauge::TTankMeasument tmp = tanksMeasument[tankNumber];
        QString queryText = "INSERT INTO TANKSMEASUMENTS (TANK_NUMBER, DATE_TIME, VOLUME, MASS, DENSITY, TCCORRECT, HEIGHT, WATER, TEMP) "
                            " VALUES ("
                            + QString::number(tankNumber) + ", " +
                            "'" + tmp.dateTime.toString("yyyy-MM-dd hh:mm:ss.zzz") + "', " +
                            QString::number(tmp.volume, 'f', 0) + ", " +
                            QString::number(tmp.mass, 'f', 0) + ", " +
                            QString::number(tmp.density, 'f', 1) + ", " +
                            QString::number(tmp.TKCorrect, 'f', 2) + ", " +
                            QString::number(tmp.height, 'f', 0) + ", " +
                            QString::number(tmp.water, 'f', 1) + ", " +
                            QString::number(tmp.temp, 'f', 1) + ")";

        DBQueryExecute(queryText);
    }  
}

void TLevelGaugeMonitoring::saveTanksConfigToDB(const TLevelGauge::TTanksConfigs& tanksConfig)
{
    Q_ASSERT(_levelGauge != nullptr);
    Q_ASSERT(!tanksConfig.isEmpty());

    if (tanksConfig.isEmpty()) {
        sendLogMsg(MSG_CODE::CODE_INFORMATION, "No data about tanks config from the level gauge");
        return; //если нечего записывать - выходим
    }

    for (const auto& numberTank : tanksConfig.keys()) {
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

        DBQueryExecute(queryText);
    } 
}

void TLevelGaugeMonitoring::DBQueryExecute(const QString &queryText)
{
   if (!_db.isOpen()) {
       QString msg = QString("Cannot query to DB execute because connetion is closed. Query: %1").arg(queryText);
       saveLogToFile(msg);
       qCritical() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
       return;
   }

    QSqlQuery query(_db);
    _db.transaction();

    if (!query.exec(queryText)) {
        errorDBQuery(query);
    }

    DBCommit();
}

void TLevelGaugeMonitoring::DBCommit()
{
  if (!_db.commit()) {
    QString msg = QString("Cannot commit trancsation. Error: %1").arg(_db.lastError().text());
    qCritical() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
    saveLogToFile(msg);

    _db.rollback();

    emit finished();
  }
}

void TLevelGaugeMonitoring::errorDBQuery(const QSqlQuery& query)
{
    QString msg = QString("Cannot execute query. Error: %1").arg(query.lastError().text());
    qCritical() << QString("%1 %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(msg);
    saveLogToFile(msg);

    _db.rollback();

    emit finished();
}

void TLevelGaugeMonitoring::getTanksMeasument(const TLevelGauge::TTanksMeasuments& tanksMeasument)
{
    saveTanksMasumentToDB(tanksMeasument);

    sendLogMsg(MSG_CODE::CODE_INFORMATION, QString("Data about measuments has been successfully received from the level gauge. "
                                           "Tanks count: %1").arg(tanksMeasument.size()));
}

void TLevelGaugeMonitoring::getTanksConfig(const TLevelGauge::TTanksConfigs& tanksConfig)
{
    saveTanksConfigToDB(tanksConfig);

    sendLogMsg(MSG_CODE::CODE_INFORMATION, QString("Data about tanks configuration has been successfully received from the level gauge. "
                                                   "Tanks count: %1").arg(tanksConfig.size()));
}

void LevelGauge::TLevelGaugeMonitoring::errorOccurredLG(const QString& msg)
{
    sendLogMsg(MSG_CODE::CODE_ERROR, QString("Error getting data from level gauge Msg: " + msg));
}


void TLevelGaugeMonitoring::sendToHTTPServer()
{
    //если ответ на предыдущий запрос еще не получен - то пропускаем этот такт таймера
    if (_sending) {
        return;
    }

    QSqlQuery query(_db);
    _db.transaction();

    QString queryText = "SELECT FIRST " + QString::number(_cnf->srv_MaxRecord()) + " " +
                       "ID, DATE_TIME, TANK_NUMBER, ENABLED, DIAMETR, VOLUME, TILT, TCCOEF, OFFSET, PRODUCT "
                       "FROM TANKSCONFIG "
                       "WHERE ID > " + QString::number(_cnf->srv_LastTankConfigID()) + " "
                       "ORDER BY ID";


    if (!query.exec(queryText)) {
       errorDBQuery(query);
    }

    //форматируем XML докумен
    QString XMLStr;
    QXmlStreamWriter XMLWriter(&XMLStr);
    XMLWriter.setAutoFormatting(true);
    XMLWriter.writeStartDocument("1.0");
    XMLWriter.writeStartElement("Root");
    XMLWriter.writeTextElement("AZSCode", _cnf->srv_UserName());
    XMLWriter.writeTextElement("ClientVersion", QCoreApplication::applicationVersion());

    while (query.next()) {
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

        _maxTanksConfigsID = std::max(_maxTanksConfigsID, query.value("ID").toULongLong());
    }

    queryText = "SELECT FIRST " + QString::number(_cnf->srv_MaxRecord()) + " " +
                "ID, TANK_NUMBER, DATE_TIME, VOLUME, MASS, DENSITY, TCCORRECT, HEIGHT, WATER, TEMP "
                "FROM TANKSMEASUMENTS "
                "WHERE ID > " + QString::number(_cnf->srv_LastTankMeasumentID()) + " "
                "ORDER BY ID";

    if (!query.exec(queryText)) {
        errorDBQuery(query);
    }

    while (query.next()) {
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

        _sendingTanksMasumentsID.push_back("'" + query.value("ID").toString() + "'");

        _maxTanksMasumentsID = std::max(_maxTanksMasumentsID, query.value("ID").toULongLong());
    }

    XMLWriter.writeEndElement(); //root
    XMLWriter.writeEndDocument();

    DBCommit();

    //отправляем запрос
    sendLogMsg(MSG_CODE::CODE_INFORMATION, QString("Sending a request. Size: %1 byte").arg(XMLStr.toUtf8().size()));

    _sending = true; //устанавливаем флаг

    emit sendHTTP(XMLStr.toUtf8());
}

void TLevelGaugeMonitoring::errorOccurredHTTP(const QString& msg)
{
    _sending = false;
    //очищаем очереди
    _sendingTanksMasumentsID.clear();
    _sendingTanksConfigsID.clear();

    sendLogMsg(MSG_CODE::CODE_ERROR, "Error sending data to HTTP server. Msg: " + msg);
}

void TLevelGaugeMonitoring::getAnswerHTTP(const QByteArray &answer)
{
    _sending = false;

    if (answer.left(2) == "OK") {
        if (!_sendingTanksMasumentsID.isEmpty()) {
            QString queryText = "DELETE FROM TANKSMEASUMENTS "
                                "WHERE ID IN (" + _sendingTanksMasumentsID.join(",") + ")";

            DBQueryExecute(queryText);
        }
        if (!_sendingTanksConfigsID.empty()) {
            QString queryText = "DELETE FROM TANKSCONFIG "
                                "WHERE ID IN (" + _sendingTanksConfigsID.join(",") + ")";

            DBQueryExecute(queryText);
        }

        //если на удаление было записей _cnf->srv_MaxRecord() значит скорее всего в БД есть еще записи
        //поэтому повторяем отправку данных
        bool neetSending = (_sendingTanksMasumentsID.size() == _cnf->srv_MaxRecord()) || (_sendingTanksConfigsID.size() == _cnf->srv_MaxRecord());
        //очищаем очереди
        _sendingTanksMasumentsID.clear();
        _sendingTanksConfigsID.clear();
        //сохраняем новые значения ID
        _cnf->set_srv_LastTankMeasumentID(_maxTanksMasumentsID);
        _cnf->set_srv_LastTankConfigID(_maxTanksConfigsID);
        _cnf->save();

        sendLogMsg(MSG_CODE::CODE_INFORMATION, "Data has been successfully sent to the server."
            " LastTankConfigID: " + QString::number(_maxTanksConfigsID) +
            " LastTankMeasumentID : " + QString::number(_maxTanksMasumentsID));

        if (neetSending) sendToHTTPServer(); //если есть еще данные для отправки - повторяем отправку данных
   }
   else {
       sendLogMsg(MSG_CODE::CODE_ERROR, "Failed to send data to the server. Server answer: " + answer);
   }
}
