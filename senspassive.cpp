#include "senspassive.h"

//Qt
#include <QCoreApplication>
//My
#include "Common/common.h"

using namespace LevelGauge;

using namespace Common;

SensPassive::SensPassive(QObject* parent)
    : TLevelGauge(parent)
    , _cnf(TConfig::config())
{
    Q_CHECK_PTR(_cnf);
}

SensPassive::~SensPassive()
{
    if (_socket != nullptr)
    {
        if (_socket->isOpen())
        {
           _socket->disconnectFromHost();
        }
        _socket->deleteLater();
    }

    if (_watchDoc != nullptr)
    {
        _watchDoc->deleteLater();
    }

    if (_getDataTimer != nullptr)
    {
        _getDataTimer->deleteLater();
    }

    if (_sendDataTimer != nullptr)
    {
        _sendDataTimer->deleteLater();
    }
}

void SensPassive::start()
{
    Q_ASSERT(_socket == nullptr);
    Q_ASSERT(_sendDataTimer == nullptr);

    //getData
    _getDataTimer = new QTimer();
    QObject::connect(_getDataTimer, SIGNAL(timeout()), SLOT(getData()));
    _getDataTimer->start(_cnf->sys_Interval() * 10);

    //WatchDoc
    _watchDoc = new QTimer();
    QObject::connect(_watchDoc, SIGNAL(timeout()), SLOT(watchDocTimeout()));
    _watchDoc->setSingleShot(true);

    //send data timer
    _sendDataTimer = new QTimer();
    QObject::connect(_sendDataTimer, SIGNAL(timeout()), SLOT(sendData()));
    _sendDataTimer->start(60000);

    getData();
}

void SensPassive::connentedSocket()
{
    //ничего не делаем
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
    if (_socket == nullptr)
    {
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
     Q_CHECK_PTR(_socket);

    emit errorOccurred("Connection timeout.");

    if (_socket != nullptr)
    {
        _socket->disconnectFromHost();
    }
}

void SensPassive::sendData()
{
    if (!_tanksMeasuments.isEmpty())
    {
        emit getTanksMeasument(_tanksMeasuments);
        _tanksMeasuments.clear();
    }
}

void SensPassive::transferReset()
{
    Q_CHECK_PTR(_socket);
    Q_CHECK_PTR(_watchDoc);

    //тормозим watchDoc
   if (_watchDoc->isActive())
   {
        _watchDoc->stop();
    }

    if (_socket != nullptr)
    {
        //закрываем соединение
        if (_socket->isOpen())
        {
            _socket->disconnectFromHost();
            _socket->waitForDisconnected(5000);
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

    for (int i = 1; i < cmd.length(); ++i)
    {
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
    Common::writeDebugLogFile("Get from LG:", QString(data.toHex('|')));

    //проверяем длину
    if (data.length() < 10)
    {
        return;
    }

    //проверяем стартовый символ
    if  (data[0] != char(0xB5))
    {
        return;
    }

    //проверяем направление отправки
    if  ((data[3] & 0b10000000) != 0b10000000)
    {
        return;
    }

    uint8_t dataType = data[4]; //опреляем тип данныхю если 0x01 - измеренияб 0x20 -конфигурация резервуара
    if (!(dataType == 0x01 || dataType == 0x20))
    {
        return;
    }

    unsigned char CRC = data[data.length() - 1]; //CRC содержиться в последнем символе

    //удаляем  последний символ (CRC)
    data.remove(data.length() - 1, 1);

    //проверяем CRC
    if (SensPassive::CRC(data) != CRC)
    {
        return;
    }

    //если дошли до сюда - значит принят пакет с информацией об уровне или конфигурации резервуара
    //начинаем парсинг

    //удаляем первый (0xB5)
    data.remove(0,1);
    //парсим пришедшие данные
    QDataStream dataStream(data);
    //первый байт - адрес устройства
    uint8_t number = 0;
    dataStream >> number;
    if (!_cnf->lg_Addresses().contains(number))
    {
        emit errorOccurred("parseTanksMeasument: Invalid tank number. Number:" + QString::number(number) + " Tank ignored.");
        return;
    }

    writeDebugLogFile("Packet is define. Start parsing.", QString("Tank: %1").arg(number));

    uint8_t length = 0;
    dataStream >> length;

    uint8_t direction;
    dataStream >> direction;

    //далее идут данные. их читаем по 4 бата. первый байт - номер параметра, потом 3 байта - данные
    if (dataType == 0x01)
    {
        if (_lastGetMeausumentsData.contains(number) && (_lastGetMeausumentsData[number].secsTo(QDateTime::currentDateTime()) < 60))
        {
                return;
        }
        TLevelGauge::TTankMeasument tmp = parseTankMeasument(dataStream);
        tmp.dateTime = QDateTime::currentDateTime();

        //проверям полученные значения
        if (!checkMeasument(number, tmp))
        {
            return;
        }
        
        _tanksMeasuments.emplace(number, tmp);
        _lastGetMeausumentsData[number] = QDateTime::currentDateTime();
    }
}

TLevelGauge::TTankMeasument SensPassive::parseTankMeasument(QDataStream &dataStream)
{
// b5|01|20|81|01|a8|64|3f|02|e9|c9|41|03|a7|df|41|04|92|e3|40|05|2e|a8|40|06|30|3d|3f|07|92|e3|40|08|00|00|00|f7
    TLevelGauge::TTankMeasument tmp;
    //данный тип уровнемера не измеряет температурную компенсацию
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
        case 0x2C:
        case 0x2D:
        case 0x26: { break; }
        default : {
            emit errorOccurred("parseTankMeasument: incorrect parametr address: " + QString::number(code, 16));
        }
        }
    }

    return tmp;
}
