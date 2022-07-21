#include "senspassive.h"

//Qt
#include <QCoreApplication>
//My
#include "common.h"

using namespace LevelGauge;

SensPassive::SensPassive(LevelGauge::TConfig* cnf, QObject* parent) :
    TLevelGauge(parent),
    _cnf(cnf)
{
    Q_ASSERT(cnf != nullptr);
}

SensPassive::~SensPassive()
{
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

void SensPassive::start()
{
    Q_ASSERT(_socket == nullptr);

    _getDataTimer = new QTimer();
    QObject::connect(_getDataTimer, SIGNAL(timeout()), SLOT(getData()));
    _getDataTimer->start(_cnf->sys_Interval() * 10);

    //WatchDoc
    _watchDoc = new QTimer();
    QObject::connect(_watchDoc, SIGNAL(timeout()), SLOT(watchDocTimeout()));
    _watchDoc->setSingleShot(true);

    getData();
}

void SensPassive::connentedSocket()
{
    emit errorOccurred(QString("Connect to level gauge success."));
}

void SensPassive::disconnentedSocket()
{
    transferReset();
}

void SensPassive::readyReadSocket()
{
    //считываем буфер
    QByteArray readBuffer = _socket->readAll();

    //парсим пришедшие данные
    parseAnswer(readBuffer);
    _watchDoc->stop();
    _watchDoc->start(60000);
}

void SensPassive::errorOccurredSocket(QAbstractSocket::SocketError)
{
    emit errorOccurred(QString("Socket error. Code: %1. Msg: %2").arg(_socket->error()).arg(_socket->errorString()));
    transferReset();
}

void SensPassive::getData()
{
    //Проверяем все ли хорошо. если _socket != nuppltr значит передача идет и ничего не делаем. Если нет - зоздаем его и мытаемся подключиться
    if (_socket == nullptr) {
        _socket = new QTcpSocket;
        QObject::connect(_socket, SIGNAL(connected()), SLOT(connentedSocket()));
        QObject::connect(_socket, SIGNAL(readyRead()), SLOT(readyReadSocket()));
        QObject::connect(_socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), SLOT(errorOccurredSocket(QAbstractSocket::SocketError)));
        QObject::connect(_socket, SIGNAL(disconnected()), SLOT(disconnentedSocket()));
        //подлючаемся к уровнемеру
        _socket->connectToHost(_cnf->lg_Host(), _cnf->lg_Port(), QIODeviceBase::ReadWrite, QAbstractSocket::IPv4Protocol);
        //далее ждем conneted() или errorOccurred()
        //запускаем watchDoc
        _watchDoc->start(60000);
    }
}

void SensPassive::watchDocTimeout()
{
    Q_ASSERT(_socket != nullptr);

    emit errorOccurred("Connection timeout.");

    if (_socket != nullptr) {
        _socket->disconnectFromHost();
    }
}

void SensPassive::transferReset()
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

}

unsigned char SensPassive::CRC(const QByteArray &cmd)
{
    // на входе пакет данных без CRC
    unsigned char res = 0;

    for (int i = 1; i < cmd.length(); ++i) {
        res += cmd[i];
    }

    return res;
}

float SensPassive::float24ToFloat32(const QByteArray &number)
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

void SensPassive::parseAnswer(QByteArray data)
{
    //проверяем пакет и выходим если он нам не подходит
    LevelGauge::writeDebugLogFile("Get from LG:", QString(data.toHex('|')));

    //проверяем длину
    if (data.length() < 10) {
        return;
    }

    //проверяем стартовый символ
    if  (data[0] != char(0xB5)) {
        return;
    }

    //проверяем направление отправки
    if  ((data[3] & 0b10000000) != 0b10000000) {
        return;
    }

    uint8_t dataType = data[4]; //опреляем тип данныхю если 0x01 - измеренияб 0x20 -конфигурация резервуара
    if (!(dataType == 0x01 || dataType == 0x20)) {
        return;
    }

    unsigned char CRC = data[data.length() - 1]; //CRC содержиться в последнем символе

    //удаляем  последний символ (CRC)
    data.remove(data.length() - 1, 1);

    //проверяем CRC
    if (SensPassive::CRC(data) != CRC) {
        return;
    }

    //если дошли до сюда - значит принят пакет с информацией об уровне или конфигурации резервуара
    //начинаем парсинг

    LevelGauge::writeDebugLogFile("Packet is define. Start parsing.:", QString());

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

TLevelGauge::TTankMeasument SensPassive::parseTankMeasument(QDataStream &dataStream)
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

TLevelGauge::TTankConfig SensPassive::parseTankConfig(QDataStream &dataStream)
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

