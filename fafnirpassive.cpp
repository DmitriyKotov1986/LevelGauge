//Qt
#include <QCoreApplication>

//My
#include "Common/common.h"

#include "fafnirpassive.h"

using namespace LevelGauge;
using namespace Common;

FafnirPassive::FafnirPassive(QObject* parent)
    : TLevelGauge(parent)
    , _cnf(TConfig::config())
{
    Q_CHECK_PTR(_cnf);
}

FafnirPassive::~FafnirPassive()
{

    if (_socket != nullptr)
    {
        if (_socket->isOpen())
        {
           _socket->abort();
        }

        delete _socket;;
    }

    delete _getDataTimer;
}

void FafnirPassive::start()
{
    Q_ASSERT(_socket == nullptr);

    _getDataTimer = new QTimer();
    QObject::connect(_getDataTimer, SIGNAL(timeout()), SLOT(getData()));

    //readyReadSocket();

    connectToLevelGauge();
}

void FafnirPassive::connectToLevelGauge()
{
     Q_ASSERT(_socket == nullptr);

     _socket = new QTcpSocket;
     QObject::connect(_socket, SIGNAL(connected()), SLOT(connentedSocket()));
     QObject::connect(_socket, SIGNAL(readyRead()), SLOT(readyReadSocket()));
     QObject::connect(_socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), SLOT(errorOccurredSocket(QAbstractSocket::SocketError)));
     QObject::connect(_socket, SIGNAL(disconnected()), SLOT(disconnentedSocket()));
     //подлючаемся к уровнемеру
     _socket->connectToHost(_cnf->lg_Host(), _cnf->lg_Port(), QIODeviceBase::ReadWrite, QAbstractSocket::IPv4Protocol);
     //далее ждем conneted() или errorOccurred()
}

void FafnirPassive::connentedSocket()
{
    Q_CHECK_PTR(_getDataTimer);

    _getDataTimer->start(_cnf->sys_Interval());
}

void FafnirPassive::errorOccurredSocket(QAbstractSocket::SocketError)
{
    emit errorOccurred(QString("Socket error. Code: %1. Msg: %2").arg(_socket->error()).arg(_socket->errorString()));

    if (_getDataTimer->isActive())
    {
        _socket->disconnectFromHost();
    }
    else
    {
        transferReset();
    }
}

void FafnirPassive::disconnentedSocket()
{
    transferReset();
}

void FafnirPassive::transferReset()
{
    Q_CHECK_PTR(_socket);

    _getDataTimer->stop();

    //закрываем соединение
    _socket->deleteLater();
    _socket = nullptr;

    _readBuffer.clear();

    QTimer::singleShot(60000, [this](){ connectToLevelGauge(); });
}

float FafnirPassive::parseFloatValue(QTextStream &textStream, bool* ok /* = nullptr */)
{
    const auto str = textStream.read(8);
    if (str.length() != 8)
    {
        if (ok)
        {
            *ok = false;
        }

        return -1.0;
    }

    QByteArray data = QByteArray::fromHex(str.toUtf8());
    std::reverse(data.begin(), data.end());

    *ok = true;

    return *(reinterpret_cast<const float*>(data.constData()));
}

void FafnirPassive::readyReadSocket()
{
    _getDataTimer->stop();
    _getDataTimer->start(_cnf->sys_Interval());

    //считываем буфер
    _readBuffer += _socket->readAll();
//        _readBuffer.push_back(char(0x01));
//        _readBuffer.push_back("i21400240425181001100000649F43CF649CCCFB44451A4A1462C8F35000000004189000002200000649DCA75D49B8834F44510D74461C832A000000004198A40003300000649C4EE9F498FF96F4436C584460BDD5100000000419BBC00044000006488F2B50485021DB4435B80244CAA3A00000000041A0DC00055000006489C494F4862985F44353BE744DCDA3F00000000419B7C0006600000649293CC9490C1021444EE732460055F50000000041869C00077000006471353ED46EC69B04448958143DF28A000000000418D0C000880000064722B86F4702F04B44492BD243F6F7C2000000004195140009900000646F26D2246C7A19D444DDE3843B81C4B00000000418E4800100000006495C86D54922A0B844385CEB4627634D00000000418DF80011000000648D7C91D489FD8274439305845A3BD0A000000004199FC00120000006484B44FD48179FA7443A7B28451A14520000000041A8D400&&6BCD");
//        _readBuffer.push_back(char(0x03));
    if (_readBuffer.size() > 0)
    {
        const qsizetype posStart = _readBuffer.indexOf(char(0x01)); //пришел символ <SOH>
        if (posStart < 0)
        {
            return;
        }

        _readBuffer.remove(0, posStart + 1); //удаляем все до i в начале

        const qsizetype posEnd = _readBuffer.indexOf(char(0x03)); //пришел символ <ETX>
        if (posEnd < 0)
        {
            return;
        }

        const QByteArray data(_readBuffer.begin(), posEnd);
        _readBuffer.remove(0, posEnd + 1);

        parseAnswer(data);
    }
}

void FafnirPassive::parseTanksMeasument(const QByteArray& data)
{
    Q_ASSERT(_tanksMeasuments.isEmpty());
/*
    i21401240125160301 1 0000 06 470F5F63 46C5DC6F 442C8189 43D7A005 00000000 3E8C0000 &&F056

Typical Response Message, Computer Format:
<SOH>i214TTYYMMDDHHmmTTpssssNNFFFFFFFF...
 TTpssssNNFFFFFFFF...&&CCCC<ETX>
Notes:
1. YYMMDDHHmm - Current Date and Time
2. TT - Tank Number (Decimal, 00=all)
3. p - Product Code (single ASCII character [20h-7Eh])
4. ssss - Tank Status Bits:
 Bit 1=(LSB) Delivery in Progress Bit 2=Leak test in Progress Bit 3=Invalid Fuel Height Alarm (MAG Probes Only) Bit 4-16 - Unused
5. NN - Number of eight character Data Fields to follow (Hex)
6. FFFFFFFF - ASCII Hex IEEE float:
 1. Volume 2. Mass 3. Density 4. Height 5. Water 6. Temperature 7. TC Density 8. TC Volume 9. Ullage 10. Water Volume
7. && - Data Termination Flag
8. CCCC - Message Checksum

*/
    QTextStream textStream(data.first(data.length() - 6));
    textStream.read(16); // skip first 16 byte (i214TTYYMMDDHHmm)

    bool ok = true;
    try
    {
        while (!textStream.atEnd())
        {
            TLevelGauge::TTankMeasument tmp;

            //tank number
            const quint16 number = textStream.read(2).toUShort(&ok, 10); //2 bytes
            if (!ok || number < 1 || number > MAX_TANK_NUMBER)
            {
                throw TParseException(QString("Incorrect Tank number. Tank: %1").arg(number));
            }

            //Product
            textStream.read(1); //здесь записан имя продукта, но мы ее игнорируем

            //Tank Status Bits
            textStream.read(4);

            //Number of eight character Data Fields to follow (Hex)
            const quint8 countFloat = textStream.read(2).toUShort(&ok, 16);
            if (!ok || countFloat < 6)
            {
                throw TParseException(QString("Incorrect Number of eight character Data Fields to follow. Tank: %1 Value: %2").arg(number).arg(countFloat));
            }

            //Volume
            tmp.volume = static_cast<qint32>(parseFloatValue(textStream, &ok));
            if (!ok)
            {
                throw TParseException(QString("Incorrect Volume format. Tank: %1").arg(number));
            }

            //Mass
            tmp.mass = static_cast<qint32>(parseFloatValue(textStream, &ok));
            if (!ok)
            {
                throw TParseException(QString("Incorrect Volume format. Tank: %1").arg(number));
            }

            //Density
            tmp.density = parseFloatValue(textStream, &ok);
            if (!ok)
            {
                throw TParseException(QString("Incorrect Density format. Tank: %1").arg(number));
            }

            //Height
            tmp.height  = static_cast<qint16>(parseFloatValue(textStream, &ok));
            if (!ok)
            {
                throw TParseException(QString("Incorrect Height format. Tank: %1").arg(number));
            }

            //Water
            tmp.water = parseFloatValue(textStream, &ok);
            if (!ok)
            {
                throw TParseException(QString("Incorrect Water format. Tank: %1").arg(number));
            }

            //Temp
            tmp.temp = parseFloatValue(textStream, &ok);
            if (!ok)
            {
                throw TParseException(QString("Incorrect Volume format. Tank: %1").arg(number));
            }

            for (quint16 i = 6; i < countFloat; ++i)
            {
                textStream.read(8);
            }

            tmp.dateTime = QDateTime::currentDateTime();

            //проверям полученные значения
            if (!checkMeasument(number, tmp))
            {
                continue;
            }

            _tanksMeasuments.insert(number, tmp);
        }
    }

    catch (const TParseException& err)
    {
        emit errorOccurred(QString::fromStdString(err.what()));
    }

    if (!ok)
    {
        Common::writeLogFile("PARSE ERR", QString("Data from level gauge:\n%1").arg(data));
    }
}

void FafnirPassive::parseAnswer(const QByteArray &data)
{
    writeDebugLogFile("LG answer:", QString(data));

    if (data.length() < 79)
    {
        emit errorOccurred("Data packet is too short");

        return;
    }

    const QString cmd(data.left(4));

    if (cmd == "i201")
    {
        //skip
        return;
    }
    else if (cmd == "i214")
    {
        parseTanksMeasument(data);
        //данные готовы к отправке
        if (!_tanksMeasuments.isEmpty())
        {
            emit getTanksMeasument(_tanksMeasuments);
            _tanksMeasuments.clear();
        }
        return;
    }

    emit errorOccurred("Level gauge return: Undefine command");
}


void FafnirPassive::getData()
{
    QByteArray cmd;
    cmd.push_back(char(0x01));
    cmd.push_back("i21400");
    cmd.push_back(char(0x03));
    _socket->write(cmd);
    _socket->waitForBytesWritten(5000);

    emit errorOccurred("Timeout get data from level gauge. Try send cmd: i21400");

    writeDebugLogFile("LG request:", QString(cmd));
}


