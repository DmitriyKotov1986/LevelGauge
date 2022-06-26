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

    QObject::connect(this, SIGNAL(send(const QByteArray&)), _HTTPQuery, SLOT(sendHTTP(const QByteArray&)));
    QObject::connect(_HTTPQuery, SIGNAL(getAnswer(const QByteArray&)), SLOT(getAnswerHTTP(const QByteArray&)));
    QObject::connect(_HTTPQuery, SIGNAL(errorOccurred(const QString&)), SLOT(errorOccurredHTTP(const QString&)));

    QObject::connect(this, SIGNAL(finished()), HTTPQueryThread, SLOT(quit())); //сигнал на завершение
    QObject::connect(HTTPQueryThread, SIGNAL(finished()), HTTPQueryThread, SLOT(deleteLater())); //уничтожиться после остановки

    HTTPQueryThread->start();

    //создаем поток обработки консоли
    _console = TConsole::Console(this);
    QObject::connect(_console, SIGNAL(getCommand(const QString&)), this, SLOT(getCommand(const QString&)));

   //подключаем уровнемер
    _levelGauge = loadLG();
    QThread* levelGaugeThread = new QThread(this);
    _levelGauge->moveToThread(levelGaugeThread);

    QObject::connect(this, SIGNAL(startLG()), _levelGauge, SLOT(start()));
    QObject::connect(_levelGauge, SIGNAL(getTanksMeasument(const TLevelGauge::TTanksMeasument&)),
                     SLOT(getTanksMeasument(const TLevelGauge::TTanksMeasument&)));
    QObject::connect(_levelGauge, SIGNAL(getTanksConfig(const TLevelGauge::TTanksConfig&)),
                     SLOT(getTankConfig(const TLevelGauge::TTanksConfig&)));
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
    Q_ASSERT(_console != nullptr);
    Q_ASSERT(_levelGauge != nullptr);
    Q_ASSERT(_sendHTTPTimer != nullptr);
    Q_ASSERT(_HTTPQuery != nullptr);

    sendLogMsg(MSG_CODE::CODE_OK, "Successfully finished");

    if (_console != nullptr) {
        _console->stop();
        _console->wait(1000);
        _console->deleteLater();
    }

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
    Q_ASSERT(_console != nullptr);
    Q_ASSERT(_HTTPQuery != nullptr);
    Q_ASSERT(_levelGauge != nullptr);
    Q_ASSERT(_sendHTTPTimer != nullptr);

    //подключаемся к БД
    if (!_db.open()) {
        QString msg = QString("%1 Cannot connect to database. Error: %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(_db.lastError().text());
        qCritical() <<  msg;
        saveLogToFile(msg);

        emit finished();
    };

    //Подключаем обработку приема консольных команд
    _console->start(QThread::LowPriority);

    //запускаем опрос уровнемеров
    emit startLG();

    //запукаем таймер отправки HTTP запроов
    _sendHTTPTimer->start();

    sendLogMsg(MSG_CODE::CODE_OK, "Successfully started");
}

TLevelGauge *TLevelGaugeMonitoring::loadLG()
{
    switch (_cnf->lg_TLS()) {
    case 2: return new TLS2(_cnf);
   // case 4: return new TLS4(_cnf);
    default: {
        QString msg = "Undefine type of level gauge. Check blok [LEVELGAUGE]/'TLS' in config file";
        saveLogToFile(msg);
        //тут еще цикл обработки событий не запущен - поэтому просто выходим
        exit(-2);
    }
    }
    return nullptr;
}


void TLevelGaugeMonitoring::sendLogMsg(uint16_t category, const QString& msg)
{
    if (_cnf->sys_DebugMode()) {
        qDebug() << QTime::currentTime().toString("hh:mm:ss") << msg << timeFromStart();
    }

    QString queryText = "INSERT INTO LOG (CATEGORY, SENDER, MSG) VALUES ( "
                        + QString::number(category) + ", "
                        "\'LevelGauge\', "
                        "\'" + msg +"\'"
                        ")";

    DBQueryExecute(queryText);
}

void TLevelGaugeMonitoring::saveLogToFile(const QString& msg)
{
  if (_cnf->sys_DebugMode()) {
    QFile f(QCoreApplication::applicationDirPath() + "/Log/LevelGauge.log");
    f.open(QFile::Append);
    f.write(msg.toUtf8() + "\n");
    f.close();
  }
}

void TLevelGaugeMonitoring::saveTanksMasumentToDB(const TLevelGauge::TTanksMeasuments& tanksMeasument)
{
    Q_ASSERT(_levelGauge != nullptr);
    Q_ASSERT(!tanksMeasument.isEmpty());

    if (tanksMeasument.isEmpty()) {
        sendLogMsg(MSG_CODE::CODE_INFORMATION, "No data about measuments from the level gauge");
        return; //если нечего записывать - выходим
    }

    QString queryText = "INSERT INTO TANKSMEASUMENTS (TANK_NUMBER, DATE_TIME, VOLUME, MASS, DENSITY, TCCORRECT, HEIGHT, WATER, TEMP) "
                        " VALUES ";
    for (const auto& tankNumber : tanksMeasument.keys()) {
        TLevelGauge::TTankMeasument tmp = tanksMeasument[tankNumber];
        queryText += "(" + QString::number(tankNumber) + ", " +
                     "'" + tmp.dateTime.toString("yyyy-MM-dd hh:mm:ss.zzz") + "', " +
                     QString::number(tmp.volume, 'f', 0) + ", " +
                     QString::number(tmp.mass, 'f', 0) + ", " +
                     QString::number(tmp.density, 'f', 1) + ", " +
                     QString::number(tmp.TKCorrect, 'f', 2) + ", " +
                     QString::number(tmp.height, 'f', 0) + ", " +
                     QString::number(tmp.water, 'f', 1) + ", " +
                     QString::number(tmp.temp, 'f', 1) + "), ";
    }

    queryText.chop(2); //удаляем конечный ", "

    DBQueryExecute(queryText);
}

void TLevelGaugeMonitoring::saveTanksConfigToDB(const TLevelGauge::TTanksConfigs& tanksConfig)
{
    Q_ASSERT(_levelGauge != nullptr);
    Q_ASSERT(!tanksConfig.isEmpty());

    if (tanksConfig.isEmpty()) {
        sendLogMsg(MSG_CODE::CODE_INFORMATION, "No data about tanks config from the level gauge");
        return; //если нечего записывать - выходим
    }

    QString queryText = "INSERT INTO TANKSCONFIG (TANK_NUMBER, DATE_TIME, ENABLED, DIAMETR, VOLUME, TILT, TCCOEF, OFFSET, PRODUCT) "
                        "VALUES ";
    for (const auto& numberTank : tanksConfig.keys()) {
        TLevelGauge::TTankConfig tmp = tanksConfig[numberTank];
        queryText = "(" + QString::number(numberTank) + ", " +
                    "'" + tmp.dateTime.toString("yyyy-MM-dd hh:mm:ss.zzz") + "', " +
                    QString::number(tmp.enabled) + ", " +
                    QString::number(tmp.diametr, 'f', 0) + ", " +
                    QString::number(tmp.volume, 'f', 0) + ", " +
                    QString::number(tmp.tilt, 'f', 1) + ", " +
                    QString::number(tmp.TCCoef, 'f', 2) + ", " +
                    QString::number(tmp.offset, 'f', 0) + ", " +
                    "'" + tmp.product + "'), ";
    }

    queryText.chop(2); //удаляем конечный ", "

    DBQueryExecute(queryText);
}

void TLevelGaugeMonitoring::DBQueryExecute(const QString &queryText)
{
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
    QString msg = QString("%1 FAIL Cannot commit transation. Error: %2").arg(QTime::currentTime().toString("hh:mm:ss")).arg(_db.lastError().text());
    qCritical() << msg;
    saveLogToFile(msg);

    _db.rollback();

    emit finished();
  }
}

void TLevelGaugeMonitoring::errorDBQuery(const QSqlQuery& query)
{
    QString msg = QString("%1 FAIL Cannot execute query. Error: %2 Query: %3").arg(QTime::currentTime().toString("hh:mm:ss")).
        arg(query.lastError().text()).arg(query.lastQuery());
    qCritical() << "FAIL Cannot execute query. Error:" << query.lastError().text() << " Query:"<< query.lastQuery();
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
    sendLogMsg(MSG_CODE::CODE_ERROR, QString("Error getiing data from level gauge Msg: " + msg));
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

    #ifdef QT_DEBUG
        QFile file("LG.xml");
        if (file.open(QIODevice::Append)) {
            file.write(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz").toUtf8() + ":\r\n");
            file.write(XMLStr.toUtf8());
            file.close();
        }
    #endif

    //отправляем запрос
    if (_cnf->sys_DebugMode()) {
        qDebug() << "Sending a request. Size: " << XMLStr.toUtf8().size() << "Byte " << timeFromStart();
    }

    _sending = true; //устанавливаем флаг

    emit sendHTTP(XMLStr.toUtf8());
}


void TLevelGaugeMonitoring::getCommand(const QString &cmd)
{
    if (cmd == "QUIT\n") {   //если пришла команда QUIT - то выходим
        qDebug() << "OK";
        emit finished();
    }
}

void TLevelGaugeMonitoring::errorOccurredHTTP(const QString& msg)
{
    _sending = false;
    //очищаем очереди
    _sendingTanksMasumentsID.clear();
    _sendingTanksConfigsID.clear();

    sendLogMsg(MSG_CODE::CODE_ERROR, "Error sending data to HTTP server. Msg: " + msg);
}

QString TLevelGaugeMonitoring::timeFromStart() const
{
    return QString("%1ms").arg(_timer.secsTo(QTime::currentTime()));
}

void TLevelGaugeMonitoring::getAnswerHTTP(const QByteArray &answer)
{
    if (_cnf->sys_DebugMode()) {
        qDebug() << "Get a response from the server" << timeFromStart();
        qDebug() << "->Answer: " << answer;
    }

   _sending = false;

   if (answer.left(2) == "OK") {
        QString queryText = "DELETE FROM TANKSMEASUMENTS "
                            "WHERE ID IN (" + _sendingTanksMasumentsID.join(",") + ")";

        DBQueryExecute(queryText);

        queryText = "DELETE FROM TANKSCONFIG "
                    "WHERE ID IN (" + _sendingTanksConfigsID.join(",") + ")";

        DBQueryExecute(queryText);

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
