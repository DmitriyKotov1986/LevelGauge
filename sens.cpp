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
//    Q_ASSERT(_watchDoc != nullptr);

    if (_socket != nullptr) {
        if (_socket->isOpen()) {
           _socket->disconnectFromHost();
        }
        _socket->deleteLater();
    }

    if (_watchDoc != nullptr) {
        _watchDoc->deleteLater();
    }

    if (_getDataTimer != nullptr) {
        _getDataTimer->deleteLater();
    }
}

void Sens::start()
{
    Q_ASSERT(_socket == nullptr);
//    Q_ASSERT(_watchDoc == nullptr);

    _getDataTimer = new QTimer();
    QObject::connect(_getDataTimer, SIGNAL(timeout()), SLOT(getData()));
    _getDataTimer->start(_cnf->sys_Interval());

    //WatchDoc
    _watchDoc = new QTimer();
    QObject::connect(_watchDoc, SIGNAL(timeout()), SLOT(watchDocTimeout()));
    _watchDoc->setSingleShot(true);

    getData();
}

void Sens::connentedSocket()
{
    sendNextCmd();
    //ответ придет в readData()
}

void Sens::disconnentedSocket()
{
    transferReset();
}

void Sens::readyReadSocket()
{
    //"\xB5\x01\x04\x8F\x03\xB6\xE1" - error message

    //считываем буфер
    readBuffer += _socket->readAll();
    parseAnswer(readBuffer);

    sendNextCmd();
}

void Sens::errorOccurredSocket(QAbstractSocket::SocketError)
{
    emit errorOccurred(QString("Socket error. Code: %1. Msg: %2").arg(_socket->error()).arg(_socket->errorString()));
    transferReset();
}

void Sens::getData()
{
    //раз в 100 тактов запрашиваем конфигурацию резервуаров
    if (((tick % 100) == 0) || (tick == 0)) {
        upDataTanksConfigs();
    }
    upDataTanksMeasuments();
    sendCmd("");

    ++tick;
}

void Sens::watchDocTimeout()
{
    Q_ASSERT(_socket != nullptr);

    emit errorOccurred("Connection timeout.");

    if (_socket != nullptr) {
        _socket->disconnectFromHost();
    }
}

void Sens::sendCmd(const QByteArray &cmd)
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
            //запускаем watchDoc
            _watchDoc->start(45000);
        }
    }
    else {
        cmdQueue.enqueue(cmd);
    }
}

void Sens::sendNextCmd()
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

        //qDebug() << "Send>>" << cmd.toHex('|');
        _socket->write(cmd);

        writeDebugLogFile("LG request:", QString(cmd.toHex('|')));
    }
}

void Sens::transferReset()
{
    Q_ASSERT(_socket != nullptr);
    Q_ASSERT(_watchDoc != nullptr);

    //тормозим watchDoc
   if (_watchDoc->isActive()) {
        _watchDoc->stop();
    }

    if (_socket != nullptr) {
        //закрываем соединение
        if (_socket->isOpen()) {
            _socket->disconnectFromHost();
           //тут может второй раз прилететь событие disconnect()
        }

        _socket->deleteLater();
        _socket = nullptr;
    }

    readBuffer.clear();
}

void Sens::upDataTanksConfigs()
{
    for (const auto& addressItem: _cnf->lg_Addresses()) {
        QByteArray cmd;
        cmd.push_back(char(0xB5));         // <-start
        cmd.push_back(char(addressItem));  // <-address
        cmd.push_back(char(0x01));         // <-data length
        cmd.push_back(char(0b00000010));   // <-command
        cmd.push_back(char(0x01));
        cmd.push_back(CRC(cmd));           // <-CRC

        sendCmd(cmd);
    }
}

void Sens::upDataTanksMeasuments()
{
    for (const auto& addressItem: _cnf->lg_Addresses()) {
        QByteArray cmd;
        cmd.push_back(char(0xB5));         // <-start
        cmd.push_back(char(addressItem));  // <-address
        cmd.push_back(char(0x01));         // <-data length
        cmd.push_back(char(0b00000001));   // <-command
        cmd.push_back(char(0x01));
        cmd.push_back(CRC(cmd));           // <-CRC

        sendCmd(cmd);
    }
}

unsigned char Sens::CRC(const QByteArray &cmd)
{
    // на входе пакет данных без CRC
    unsigned char res = 0;

    for (int i = 1; i < cmd.length(); ++i) {
        res += cmd[i];
    }

    return res;
}

float Sens::float24ToFloat32(const QByteArray &number)
{
    Q_ASSERT(number.length() == 3);
    unsigned char tmp[4]; // промежуточный массив
    float out; // обычный float
    // обнуляем
    memset(tmp, 0x00, 4);
    // копируем полученные 24 бита
    memcpy(tmp + 1, number.data(), 3);
    // копируем во float
    memcpy(&out, tmp, sizeof(float));
    // теперь с out можно работать как с обычным float
    return out;
}

void Sens::parseAnswer(QByteArray data)
{
    //qDebug() << "Answer>>" << data.toHex('|');

    LevelGauge::writeDebugLogFile("LG answer:", QString(data.toHex('|')));

    //проверяем длину
    if (data.length() < 10) {
        emit errorOccurred("parseAnswer: message so short. Data size:" + QString::number(data.size()) + ". Message ignored.");
        return;
    }

    //проверяем стартовый символ
    if  (data[0] != char(0xB5)) {
        emit errorOccurred("parseAnswer: can not find start data message. Data size:" + QString::number(data.size()) + ". Message ignored.");
        return;
    }

    //проверяем направление отправки
    if  ((data[3] & 0b10000000) != 0b10000000) {
        emit errorOccurred("parseAnswer: sender is not levelgauge. Message ignored.");
        return;
    }


    uint8_t dataType = data[4]; //опреляем тип данныхю если 0x01 - измеренияб 0x20 -конфигурация резервуара
    if (!(dataType == 0x01 || dataType == 0x20)) {
        emit errorOccurred("parseAnswer: Undefine data type. Message ignored.");
        return;
    }

    unsigned char CRC = data[data.length() - 1]; //CRC содержиться в последнем символе

    //удаляем  последний символ (CRC)
    data.remove(data.length() - 1, 1);

    //проверяем CRC
    if (Sens::CRC(data) != CRC) {
        emit errorOccurred("parseAnswer: incorrect CRC. Message ignored. Packet: " + QString::number(CRC, 16) + " func: " + QString::number(Sens::CRC(data), 16));
        return;
    }
    //удаляем первый (0xB5)
    data.remove(0,1);
    //парсим пришедшие данные
    QDataStream dataStream(data);
    //первый байт - адрес устройства
    uint8_t number = 0;
    dataStream >> number;
    if ((number < 1) || (number > 6)) {
        emit errorOccurred("parseTanksMeasument: Invalid tank number. Number:" + QString::number(number) + " Tank ignored.");
        return;
    }

    uint8_t length = 0;
    dataStream >> length;

    uint8_t direction;
    dataStream >> direction;

    //далее идут данные. их читаем по 4 бата. первый байт - номер параметра, потом 3 байта - данные
    if (dataType == 0x01) {
        TLevelGauge::TTankMeasument tmp = parseTankMeasument(dataStream);
        //проверям полученные значения
        if ((tmp.volume < 10) || (tmp.volume > 10000000)) {
            emit errorOccurred("parseTanksMeasument: Tank:" + QString::number(number) + " Invalid value received. Volume:" + QString::number(tmp.volume) + " Tank ignored.");
            return;
        }
        if ((tmp.mass) < 10 || (tmp.mass > 10000000)) {
            emit errorOccurred("parseTanksMeasument: Tank:" + QString::number(number) + " Invalid value received. Mass:" + QString::number(tmp.mass) + " Tank ignored.");
            return;
        }
        if ((tmp.density < 500.0) || (tmp.density > 1200.0)) {
            emit errorOccurred("parseTanksMeasument: Tank:" + QString::number(number) + " Invalid value received. Density:" + QString::number(tmp.density) + " Tank ignored.");
            return;
        }
        if ((tmp.height < 10) || (tmp.height > 20000)) {
            emit errorOccurred("parseTanksMeasument: Tank:" + QString::number(number) + " Invalid value received. Height:" + QString::number(tmp.height) + " Tank ignored.");
            return;
        }
        if ((tmp.temp < -60.0)||(tmp.temp > 60.0)) {
            emit errorOccurred("parseTanksMeasument: Tank:" + QString::number(number) + " Invalid value received. Temp:" + QString::number(tmp.temp) + " Tank ignored.");
            return;
        }
        tmp.dateTime = QDateTime::currentDateTime();

        _tanksMeasuments.emplace(number, tmp);

        emit getTanksMeasument(_tanksMeasuments);
        _tanksMeasuments.clear();
    }
    else if (dataType == 0x20) {
        TLevelGauge::TTankConfig tmp = parseTankConfig(dataStream);
        //проверям полученные значения
        if (((tmp.diametr < 10) || (tmp.diametr > 20000))  && (_tanksConfigs.contains(number) && _tanksConfigs[number].enabled)){
            emit errorOccurred("parseTanksDiametr: Tank:" + QString::number(number) + " Invalid value received. Diametr:" + QString::number(tmp.diametr) + " Value ignored.");
            return;
        }
        if (((tmp.volume < 10) || (tmp.volume > 10000000)) && (_tanksConfigs.contains(number) && _tanksConfigs[number].enabled)) {
            emit errorOccurred("parseTanksVolume: Tank:" + QString::number(number) + " Invalid value received. Volume:" + QString::number(tmp.volume) + " Value ignored.");
            return;
        }
        tmp.dateTime = QDateTime::currentDateTime();

        _tanksConfigs.emplace(number, tmp);

        emit getTanksConfig(_tanksConfigs);
        _tanksConfigs.clear();
    }
}

TLevelGauge::TTankMeasument Sens::parseTankMeasument(QDataStream &dataStream)
{
// b5|01|20|81|01|a8|64|3f|02|e9|c9|41|03|a7|df|41|04|92|e3|40|05|2e|a8|40|06|30|3d|3f|07|92|e3|40|08|00|00|00|f7
    TLevelGauge::TTankMeasument tmp;
    //данный тип уровнемера не измеряет температурную компенсацию
    tmp.TKCorrect = 0;
    uint8_t code = 0xFF;
    while (!dataStream.atEnd() && (code != 0)) {
        dataStream >> code;

        char* tmpFloat = new char(3);
        dataStream.readRawData(tmpFloat, 3);
        QByteArray float24 = QByteArray::fromRawData(tmpFloat, 3);
        float value = float24ToFloat32(float24);
        delete tmpFloat;

        switch (code) {
        //уровень, м
        case 0x01: { tmp.height = value * 1000.0; break; }
        //температура, гр. ц.
        case 0x02: { tmp.temp = value; break; }
        //процент заполнения %
        case 0x03: { break; }
        //общий объем м3
        case 0x04: { break; }
        //масса т
        case 0x05: { tmp.mass = value * 1000.0; break; }
        //пллотность т/м3
        case 0x06: { tmp.density = value * 1000.0; break; }
        //объем основного продукта м3
        case 0x07: { tmp.volume = value * 1000.0; break; }
        //масса м
        case 0x08: { tmp.water = value * 1000.0; break; }
        default : {
            emit errorOccurred("parseTankMeasument: incorrect parametr address: " + QString::number(code, 16));
        }
        }
    }

    return tmp;
}

TLevelGauge::TTankConfig Sens::parseTankConfig(QDataStream &dataStream)
{
    //b5|01|3c|82|20|85|6b|3e|21|66|36|40|22|f3|fd|3c|23|83|40|3d|24|00|00|40|25|a3|30|40|26|81|cb|41|2e|39|b4|3c|35|62|de|ba|29|62|30|3f|2a|ba|49|3f|e4|99|64|43|e5|cc|cc|42|33|c0|ca|3d|34|00|00|00|c7
    TLevelGauge::TTankConfig tmp;
    //данный тип уровнемера не измеряет температурную компенсацию
    tmp.enabled = true;
    tmp.TCCoef = 0.0;
    tmp.tilt = 0.0;
    tmp.product = "na";
    tmp.offset = 0;

    uint8_t code = 0xFF;
    while (!dataStream.atEnd() && (code != 0)) {
        dataStream >> code;

        char* tmpFloat = new char(3);
        dataStream.readRawData(tmpFloat, 3);
        QByteArray float24 = QByteArray::fromRawData(tmpFloat, 3);
        float value = float24ToFloat32(float24);
        delete tmpFloat;

        switch (code) {
        //высота, м
        case 0x25: { tmp.diametr = value * 1000.0; break; }
        //объем, м3
        case 0x26: { tmp.volume = value * 1000.0; break; }
        case 0x20:
        case 0x21:
        case 0x22:
        case 0x23:
        case 0x24:
        case 0x27:
        case 0x29:
        case 0x2A:
        case 0x2B:
        case 0x2C:
        case 0x2D:
        case 0x2E:
        case 0x33:
        case 0x34:
        case 0x35:
        case 0xe4:
        case 0xe5:{ break; }

        default : {
            emit errorOccurred("parseTankConfig: incorrect parametr address: " + QString::number(code, 16));
        }
        }
    }

    return tmp;
}

