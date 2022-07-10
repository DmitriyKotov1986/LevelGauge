#include "tls4.h"

//Qt
#include <QCoreApplication>
//My
#include "common.h"

using namespace LevelGauge;

TLS4::TLS4(TConfig* cnf, QObject* parent) :
    TLevelGauge(parent),
    _cnf(cnf)
{
    Q_ASSERT(cnf != nullptr);
}

TLS4::~TLS4()
{
    Q_ASSERT(_socket != nullptr);

    if (_socket != nullptr) {
        if (_socket->isOpen()) {
           _socket->disconnectFromHost();
        }
        _socket->deleteLater();
    }

    if (_getDataTimer != nullptr) {
        _getDataTimer->deleteLater();
    }
}

void TLS4::start()
{
    Q_ASSERT(_socket == nullptr);

    _getDataTimer = new QTimer();

    QObject::connect(_getDataTimer, SIGNAL(timeout()), SLOT(getData()));

    _getDataTimer->start(_cnf->sys_Interval());

    getData();
}

void TLS4::connentedSocket()
{
    sendNextCmd();
    //ответ придет в readData()
}

void TLS4::parseTanksMeasument(const QByteArray& data)
{
    Q_ASSERT(_tanksMeasuments.isEmpty());

    QTextStream textStream(data);
    //Пропускаем 11 строчек
    skipLine(textStream, 11);
    //парсим таблицу с данными
    while (!textStream.atEnd())  {
        TLevelGauge::TTankMeasument tmp;
        uint number = 0;
        textStream >> number;
        QString IgnoreStr;
        textStream >> IgnoreStr; //здесь записан имя продукта, но мы ее игнорируем
        textStream >> tmp.volume;
        //Mass
        float currValue;
        textStream >> currValue;
        tmp.mass = static_cast<int>(currValue);
        //density
        textStream >> tmp.density;
        //height
        textStream >> currValue;
        tmp.height = static_cast<int>(currValue);

        textStream >> tmp.water;
        textStream >> tmp.temp;

        tmp.dateTime = QDateTime::currentDateTime();
        textStream.readLine();

        //проверям полученные значения
        if ((number < 1) || (number > 6)) {
            emit errorOccurred("parseTanksMeasument: Invalid tank number. Number:" + QString::number(number) + " Tank ignored.");
            continue;
        }
        if ((tmp.volume < 10) || (tmp.volume > 10000000)) {
            emit errorOccurred("parseTanksMeasument: Tank:" + QString::number(number) + " Invalid value received. Volume:" + QString::number(tmp.volume) + " Tank ignored.");
            continue;
        }
        if ((tmp.mass) < 10 || (tmp.mass > 10000000)) {
            emit errorOccurred("parseTanksMeasument: Tank:" + QString::number(number) + " Invalid value received. Mass:" + QString::number(tmp.mass) + " Tank ignored.");
            continue;
        }
        if ((tmp.density < 500.0) || (tmp.density > 1200.0)) {
            emit errorOccurred("parseTanksMeasument: Tank:" + QString::number(number) + " Invalid value received. Density:" + QString::number(tmp.density) + " Tank ignored.");
            continue;
        }
        if ((tmp.height < 10) || (tmp.height > 20000)) {
            emit errorOccurred("parseTanksMeasument: Tank:" + QString::number(number) + " Invalid value received. Height:" + QString::number(tmp.height) + " Tank ignored.");
            continue;
        }
        if ((tmp.temp < -60.0)||(tmp.temp > 60.0)) {
            emit errorOccurred("parseTanksMeasument: Tank:" + QString::number(number) + " Invalid value received. Temp:" + QString::number(tmp.temp) + " Tank ignored.");
            continue;
        }
        _tanksMeasuments.insert(number, tmp);
    }
}

void TLS4::parseTanksEnabled(const QByteArray& data)
{
    QTextStream textStream(data);
    //Пропускаем 6 строчек
    skipLine(textStream, 6);

    while (!textStream.atEnd())  {
        uint number;
        textStream >> number;
        if ((number < 1) || (number > 6)) {
            emit errorOccurred("parseTanksEnabled: Invalid tank number. Number:" + QString::number(number) + " Value ignored.");
            continue;
        }
        //считыввем продукт
        QString product;
        textStream >> product;
        //считываем вкл или выкл резервуар
        QString enabled;
        textStream >> enabled;
        _tanksConfigs[number].enabled = ((enabled == "BK") || (enabled == "ON") || (enabled == "Р'Р?"));
        if (_tanksConfigs[number].enabled) {
            _tanksConfigs[number].product = product;
        }

        textStream.readLine();
    }
}

void TLS4::parseTanksDiametr(const QByteArray& data)
{
    QTextStream textStream(data);
    //Пропускаем 6 строчек
    skipLine(textStream, 6);

    while (!textStream.atEnd())  {
        uint number;
        textStream >> number;
        if ((number < 1) || (number > 6)) {
            emit errorOccurred("parseTanksDiametr: Invalid tank number. Number:" + QString::number(number) + " Tank ignored.");
            continue;
        }
        //игнорируем название продукта
        QString ignoreTmp;
        textStream >> ignoreTmp;
        //считываем диаметр
        qint16 tmp;
        textStream >> tmp; //в случае ошибки tmp будет = 0
        if (((tmp < 10) || (tmp > 20000))  && (_tanksConfigs.contains(number) && _tanksConfigs[number].enabled)){
            emit errorOccurred("parseTanksDiametr: Tank:" + QString::number(number) + " Invalid value received. Diametr:" + QString::number(tmp) + " Value ignored.");
            continue;
        }

        _tanksConfigs[number].diametr = tmp;

        textStream.readLine();
    }
}

void TLS4::parseTanksVolume(const QByteArray& data)
{
    QTextStream textStream(data);
    //Пропускаем 6 строчек
    skipLine(textStream, 6);

    while (!textStream.atEnd())  {
        uint number;
        textStream >> number;
        if ((number < 1) || (number > 6)) {
            emit errorOccurred("parseTanksVolume: Invalid tank number. Number:" + QString::number(number) + " Tank ignored.");
            continue;
        }
        //игнорируем название продукта
        QString ignoreTmp;
        textStream >> ignoreTmp;

        qint32 tmp;
        textStream >> tmp;
        if (((tmp < 10) || (tmp > 10000000)) && (_tanksConfigs.contains(number) && _tanksConfigs[number].enabled)) {
            emit errorOccurred("parseTanksVolume: Tank:" + QString::number(number) + " Invalid value received. Volume:" + QString::number(tmp) + " Value ignored.");
            continue;
        }

        _tanksConfigs[number].volume = tmp;

        textStream.readLine();
     }
}

void TLS4::parseTanksTilt(const QByteArray& data)
{
    QTextStream textStream(data);
    //Пропускаем 6 строчек
    skipLine(textStream, 6);

    while (!textStream.atEnd())  {
        uint number;
        textStream >> number;
        if ((number < 1) || (number > 6)) {
            emit errorOccurred("parseTanksTilt: Invalid tank number. Number:" + QString::number(number) + " Tank ignored.");
            continue;
        }
        //игнорируем название продукта
        QString ignoreTmp;
        textStream >> ignoreTmp;

        float tmp;
        textStream >> tmp;
        if (((tmp < -180.0) || (tmp > 180.0)) && (_tanksConfigs.contains(number) && _tanksConfigs[number].enabled)) {
            emit errorOccurred("parseTanksTilte: Tank:" + QString::number(number) + " Invalid value received. Volume:" + QString::number(tmp) + " Value ignored.");
            continue;
        }
        _tanksConfigs[number].tilt = tmp;

        textStream.readLine();
    }
}

void TLS4::parseTanksTCCoef(const QByteArray& data)
{
    QTextStream textStream(data);
    //Пропускаем 6 строчек
    skipLine(textStream, 6);

    while (!textStream.atEnd())  {
        uint number;
        textStream >> number;
        if ((number < 1) || (number > 6)) {
            emit errorOccurred("parseTanksTCCoef: Invalid tank number. Number:" + QString::number(number) + " Tank ignored.");
            continue;
        }
        //игнорируем название продукта
        QString ignoreTmp;
        textStream >> ignoreTmp;

        float tmp;
        textStream >> tmp;
        if (((tmp < -100) || (tmp > 100)) && (_tanksConfigs.contains(number) && _tanksConfigs[number].enabled)) {
            emit errorOccurred("parseTanksTCCoef: Tank:" + QString::number(number) + " Invalid value received. Volume:" + QString::number(tmp) + " Value ignored.");
            continue;
        }
        _tanksConfigs[number].TCCoef = tmp;

        textStream.readLine();
    }
}

void TLS4::skipLine(QTextStream &stream, const int count)
{
    for (int i = 0; i < count; ++i ) stream.readLine();
}

void TLS4::parseTanksOffset(const QByteArray &data)
{
    QTextStream textStream(data);
    //Пропускаем 6 строчек
    skipLine(textStream, 6);

    while (!textStream.atEnd())  {
        uint number;
        textStream >> number;
        if ((number < 1) || (number > 6)) {
            emit errorOccurred("parseTanksTCCoef: Invalid tank number. Number:" + QString::number(number) + " Tank ignored.");
            continue;
        }

        qint16 tmp;
        textStream >> tmp;
        if (((tmp < -100) || (tmp > 1000)) && (_tanksConfigs.contains(number) && _tanksConfigs[number].enabled)) {
            emit errorOccurred("parseTankPffset: Tank:" + QString::number(number) + " Invalid value received. Volume:" + QString::number(tmp) + " Value ignored.");
            continue;
        }
        _tanksConfigs[number].offset = tmp;

        textStream.readLine();
    }
}

void TLS4::parseAnswer(QByteArray &data)
{
    LevelGauge::writeDebugLogFile("LG answer:", QString(data));

    if (data.length() < 3) {
        emit errorOccurred("Data packet is too short");
        return;
    }

    if (data.left(8) == "9999FF1B") {
        emit errorOccurred("Level gauge return: Undefine command");
        return;
    }

    data.remove(0, 2);
    data.truncate(data.length() - 4);

    QString cmd(data.left(6));

    if (cmd == "I21400") { //показания уровнемера
        parseTanksMeasument(data);
        //данные готовы к отправке
        if (!_tanksMeasuments.isEmpty()) {
            emit getTanksMeasument(_tanksMeasuments);
            _tanksMeasuments.clear();
        }
    }
    else if (cmd == "I60100") {
        parseTanksEnabled(data);
    }
    else if (cmd == "I60700") {
        parseTanksDiametr(data);
    }
    else if (cmd == "I60400") {
        parseTanksVolume(data);
    }
    else if (cmd == "I60800") {
        parseTanksTilt(data);
    }
    else if (cmd == "I60900") {
        parseTanksTCCoef(data);
    }
    else if (cmd == "I60C00") { //эта команда отправляется последней при получении геометри резервуара
        parseTanksOffset(data);
        //данные готовы к отправке
        if (!_tanksConfigs.isEmpty()) {
            //записываем дату измерения
            for (const auto& tankNumber : _tanksConfigs.keys()) {
                _tanksConfigs[tankNumber].dateTime = QDateTime::currentDateTime();
            }
            emit getTanksConfig(_tanksConfigs);
            _tanksConfigs.clear();
        }
    }
    else {
        emit errorOccurred("Undefine function in answer. Level gauge will be reboot. Data: " + data);
        sendCmd("S00100"); //перезагружаем уровнемер, если что-то пошло не так
        sendCmd("");
    }
}

void TLS4::upDateTanksConfigs()
{
    sendCmd("I60100"); //продукт и вкл
    sendCmd("I60700"); //диаметр
    sendCmd("I60400"); //объем
    sendCmd("I60800"); //наклон
    sendCmd("I60900"); //Температурный коэф
    sendCmd("I60C00"); //смещение по высоте
}

void TLS4::upDateTanksMeasuments()
{
    sendCmd("I21400"); //текущие измерения
}

void TLS4::sendCmd(const QByteArray &cmd)
{
    //если _socket еще не определен - значит передача данных еще не идет
    if (_socket != nullptr) {
        return;
    }
    //если пришла пустая команда - запускаем отрпавку данных
    if (cmd.isEmpty()) {
        if (!cmdQueue.isEmpty()) {
            _socket = new QTcpSocket;
            QObject::connect(_socket, SIGNAL(connected()), SLOT(connentedSocket()));
            QObject::connect(_socket, SIGNAL(readyRead()), SLOT(readyReadSocket()));
            QObject::connect(_socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), SLOT(errorOccurredSocket(QAbstractSocket::SocketError)));
            QObject::connect(_socket, SIGNAL(disconnected()), SLOT(disconnentedSocket()));
            //подлючаемся к уровнемеру
            _socket->connectToHost(_cnf->lg_Host(), _cnf->lg_Port(), QIODeviceBase::ReadWrite, QAbstractSocket::IPv4Protocol);
            //далее ждем conneted() или errorOccurred()
        }
    }
    else {
        cmdQueue.enqueue(char(1) + cmd + char(3));
    }
}

void TLS4::sendNextCmd()
{
    Q_ASSERT(_socket != nullptr);

    //все команды отправлены, или произошла ошибка - выходим
    if (cmdQueue.isEmpty() || (!_socket->isOpen())) {
        _socket->disconnectFromHost();
        //далее ждем сигнал disconnect()
        //отключение происходит в обработчике сигнала QTcpSocket::disconnect()  - disconnentedSocket()
    }
    else {
        //очищаем буфер
        readBuffer.clear();
        QByteArray cmd = cmdQueue.dequeue();
        _socket->write(cmd);

        writeDebugLogFile("LG request:", QString(cmd));
    }
}

void TLS4::transferReset()
{
    Q_ASSERT(_socket != nullptr);

    if (_socket->isOpen()) {
        _socket->disconnectFromHost();
    }

    _socket->deleteLater();
    _socket = nullptr;

    readBuffer.clear();
}

void TLS4::getData()
{
    //раз в 100 тактов запрашиваем конфигурацию резервуаров
    if (((tick % 100) == 0) || (tick == 0)) {
        upDateTanksConfigs();
    }
    upDateTanksMeasuments();
    sendCmd("");

    ++tick;
}

void TLS4::errorOccurredSocket(QAbstractSocket::SocketError)
{
    emit errorOccurred(QString("Socket error.Code: %1. Msg: %2").arg(_socket->error()).arg(_socket->errorString()));
    transferReset();
}

void TLS4::readyReadSocket()
{
    //считываем буфер
    readBuffer += _socket->readAll();

    if (readBuffer.size() > 0) {
        qsizetype posEXT = readBuffer.indexOf(char(3)); //пришел символ 3 - конец пакета
        if (posEXT != -1) {
            readBuffer.remove(posEXT, 1); //удаляем <EXT> в конце
            readBuffer.remove(0, 1); //Удаляем <SOH>  в начале
            parseAnswer(readBuffer);
            //отправляем слудующую
            sendNextCmd();
        }
    }
}

void TLS4::disconnentedSocket()
{
    transferReset();
}
