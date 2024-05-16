#include "sens.h"

//Qt
#include <QCoreApplication>

//My
#include "Common/common.h"

using namespace LevelGauge;

using namespace Common;

Sens::Sens(QObject* parent)
    : TLevelGauge(parent)
    , _cnf(TConfig::config())
{
    Q_CHECK_PTR(_cnf);
}

Sens::~Sens()
{
    Q_CHECK_PTR(_watchDoc);

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

    //send data timer
    _sendDataTimer = new QTimer();
    QObject::connect(_sendDataTimer, SIGNAL(timeout()), SLOT(sendData()));
    _sendDataTimer->start(60000);

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
    _readBuffer += _socket->readAll();
    parseAnswer(_readBuffer);

    sendNextCmd();
}

void Sens::errorOccurredSocket(QAbstractSocket::SocketError)
{
    emit errorOccurred(QString("Socket error. Code: %1. Msg: %2").arg(_socket->error()).arg(_socket->errorString()));
    transferReset();
}

void Sens::getData()
{
    upDataTanksMeasuments();
    sendCmd("");

    ++tick;
}

void Sens::watchDocTimeout()
{
    Q_ASSERT(_socket != nullptr);

    emit errorOccurred("Connection timeout.");

    if (_socket != nullptr)
    {
        _socket->disconnectFromHost();
    }
}

void Sens::sendData()
{
    if (!_tanksMeasuments.isEmpty())
    {
        emit getTanksMeasument(_tanksMeasuments);
        _tanksMeasuments.clear();
    }
}

void Sens::sendCmd(const QByteArray &cmd)
{
    //если _socket еще не определен - значит передача данных еще не идет
    if (_socket != nullptr)
    {
        return;
    }
    //если пришла пустая команда - запускаем отрпавку данных
    if (cmd.isEmpty())
    {
        if (!_cmdQueue.isEmpty())
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
            _watchDoc->start(45000);
        }
    }
    else {
        _cmdQueue.enqueue(cmd);
    }
}

void Sens::sendNextCmd()
{
    Q_CHECK_PTR(_socket);

    //все команды отправлены, или произошла ошибка - выходим
    if (_cmdQueue.isEmpty() || (!_socket->isOpen()))
    {
        _socket->disconnectFromHost();
        //далее ждем сигнал disconnect()
        //отключение происходит в обработчике сигнала QTcpSocket::disconnect()  - disconnentedSocket()
    }
    else
    {
        //очищаем буфер
        _readBuffer.clear();
        QByteArray cmd = _cmdQueue.dequeue();

        //qDebug() << "Send>>" << cmd.toHex('|');
        _socket->write(cmd);

        Common::writeDebugLogFile("LG request:", QString(cmd.toHex('|')));
    }
}

void Sens::transferReset()
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

    _readBuffer.clear();
}

void Sens::upDataTanksMeasuments()
{
    for (const auto& addressItem: _cnf->lg_Addresses())
    {
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

    for (int i = 1; i < cmd.length(); ++i)
    {
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

    writeDebugLogFile("LG answer:", QString(data.toHex('|')));

    //проверяем длину
    if (data.length() < 10)
    {
        emit errorOccurred("Message so short. Data size:" + QString::number(data.size()) + ". Message ignored.");
        return;
    }

    //проверяем стартовый символ
    if  (data[0] != char(0xB5))
    {
        emit errorOccurred("Can not find start data message. Data size:" + QString::number(data.size()) + ". Message ignored.");
        return;
    }

    //проверяем направление отправки
    if  ((data[3] & 0b10000000) != 0b10000000)
    {
        emit errorOccurred("Sender is not levelgauge. Message ignored.");
        return;
    }


    uint8_t dataType = data[4]; //опреляем тип данныхю если 0x01 - измеренияб 0x20 -конфигурация резервуара
    if (!(dataType == 0x01 || dataType == 0x20))
    {
        emit errorOccurred("Undefine data type. Message ignored.");
        return;
    }

    unsigned char CRC = data[data.length() - 1]; //CRC содержиться в последнем символе

    //удаляем  последний символ (CRC)
    data.remove(data.length() - 1, 1);

    //проверяем CRC
    if (Sens::CRC(data) != CRC)
    {
        emit errorOccurred("Incorrect CRC. Message ignored. Packet: " + QString::number(CRC, 16) + " func: " + QString::number(Sens::CRC(data), 16));
        return;
    }
    //удаляем первый (0xB5)
    data.remove(0,1);
    //парсим пришедшие данные
    QDataStream dataStream(data);
    //первый байт - адрес устройства
    uint8_t number = 0;
    dataStream >> number;
    if (!_cnf->lg_Addresses().contains(number))
    {
        emit errorOccurred("Invalid tank number. Number:" + QString::number(number) + " Tank ignored.");
        return;
    }

    uint8_t length = 0;
    dataStream >> length;

    uint8_t direction;
    dataStream >> direction;

    //далее идут данные. их читаем по 4 бата. первый байт - номер параметра, потом 3 байта - данные
    if (dataType == 0x01)
    {
        TLevelGauge::TTankMeasument tmp = parseTankMeasument(dataStream);
        //проверям полученные значения 
        tmp.dateTime = QDateTime::currentDateTime();
        if (!checkMeasument(number, tmp))
        {
            return;
        }

        _tanksMeasuments.emplace(number, tmp);
    }
}

TLevelGauge::TTankMeasument Sens::parseTankMeasument(QDataStream &dataStream)
{
// b5|01|20|81|01|a8|64|3f|02|e9|c9|41|03|a7|df|41|04|92|e3|40|05|2e|a8|40|06|30|3d|3f|07|92|e3|40|08|00|00|00|f7
    TLevelGauge::TTankMeasument tmp;
    //данный тип уровнемера не измеряет температурную компенсацию
    uint8_t code = 0xFF;
    while (!dataStream.atEnd() && (code != 0))
    {
        dataStream >> code;

        char* tmpFloat = new char(3);
        dataStream.readRawData(tmpFloat, 3);
        QByteArray float24 = QByteArray::fromRawData(tmpFloat, 3);
        float value = float24ToFloat32(float24);
        delete tmpFloat;

        switch (code)
        {
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
        default :
        {
            emit errorOccurred("Incorrect parametr address: " + QString::number(code, 16));
        }
        }
    }

    return tmp;
}

