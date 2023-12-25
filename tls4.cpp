#include "tls4.h"

//Qt
#include <QCoreApplication>

//My
#include "Common/common.h"

using namespace LevelGauge;
using namespace Common;

TLS4::TLS4(QObject* parent)
    : TLevelGauge(parent)
    , _cnf(TConfig::config())
{
    Q_CHECK_PTR(_cnf);
}

TLS4::~TLS4()
{
    Q_ASSERT(_watchDoc != nullptr);

    if (_socket != nullptr)
    {
        if (_socket->isOpen())
        {
           _socket->abort();
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
}

void TLS4::start()
{
    Q_ASSERT(_socket == nullptr);

    _getDataTimer = new QTimer();
    QObject::connect(_getDataTimer, SIGNAL(timeout()), SLOT(getData()));
    _getDataTimer->start(_cnf->sys_Interval());

    //WatchDoc
    _watchDoc = new QTimer();
    QObject::connect(_watchDoc, SIGNAL(timeout()), SLOT(watchDocTimeout()));
    _watchDoc->setSingleShot(true);

    getData();
}

void TLS4::connentedSocket()
{
    sendNextCmd();
    //ответ придет в readData()
}


float TLS4::parseFloatValue(QTextStream &textStream, quint8 tankNumber, const QString& parametrName)
{
    float result = 0.0f;
    QString currValue_str;
    textStream >> currValue_str; //возможные значения 1234.00 или nan
    if (currValue_str == "nan")
    {
        throw TParseException(QString("parseFloatValue: Incorrect value %1. Number: %2. Value: %3. Tank ignored")
                              .arg(parametrName)
                              .arg(tankNumber)
                              .arg(currValue_str));
    }
    else
    {
        bool ok = true;
        result = currValue_str.toFloat(&ok);
        if (!ok)
        {
            throw QString("parseTanksMeasument: Incorrect format %1. Number: %2. Value: %3. Tank ignored")
                    .arg(parametrName)
                    .arg(tankNumber)
                    .arg(currValue_str);
        }
    }

    return result;
}

qint64 TLS4::parseIntValue(QTextStream &textStream, quint8 tankNumber, const QString& parametrName)
{
    qint64 result = 0.0f;
    QString currValue_str;
    textStream >> currValue_str;

    bool ok = true;
    result = currValue_str.toLongLong(&ok);
    if (!ok)
    {
        throw QString("parseIntValue: Incorrect format %1. Number: %2. Value: %3. Tank ignored")
                .arg(parametrName)
                .arg(tankNumber)
                .arg(currValue_str);
    }

    return result;
}

void TLS4::parseTanksMeasument(const QByteArray& data)
{
    Q_ASSERT(_tanksMeasuments.isEmpty());
/*
    9999FF1B^AI21400

    I21400
    24-12-23 20:56






    ЗАПАСЫ ПО МАССЕ В РЕЗ

    РЕЗ  ПРОДУКТ                ОБЪЕМ        МАС     ПЛОТ-ТЬ     УР.  ВОДА    ТЕМП
      1  AI-96                   1879    1422.98       757.5   198.9   0.0     9.2
      2  AI-96                   2823    2121.95       751.6   261.7   0.0     8.1
      3  AI-96                   4371    2059.12       737.8   355.2 178.1     6.4
      5  AI-96                   4025    2495.03       738.7   334.4  97.2     6.6
      6  AI-96                   3572    2632.56       737.0   308.4   0.0     7.0
      7  AI-96                  33645   24892.87       739.9  1507.6   0.0     3.6
      8  AI-96                  25895        nan         nan  1217.0  66.0     3.3
      9  AI-96                  67549   49927.78       739.1  2691.8   0.0     2.5
     10  AI-96                   3241    2540.68       783.9   288.6   0.0     7.6

*/

    QTextStream textStream(data);
    //Пропускаем 11 строчек
    skipLine(textStream, 11);
    //парсим таблицу с данными
    bool ok = true;
    while (!textStream.atEnd())
    {
        uint number = 0;
        TLevelGauge::TTankMeasument tmp;
        try
        {
            //tank number
            number = parseIntValue(textStream, 0, "tank number");

            //Product
            QString IgnoreStr;
            textStream >> IgnoreStr; //здесь записан имя продукта, но мы ее игнорируем

            //Volume
            tmp.volume = parseIntValue(textStream, number, "volume");

            //Mass
            tmp.mass = static_cast<int>(parseFloatValue(textStream, number, "mass"));

            //Density
            tmp.density = parseFloatValue(textStream, number, "density");

            //Height
            tmp.height  = static_cast<int>( parseFloatValue(textStream, number, "height"));

            //Water
            tmp.water = parseFloatValue(textStream, number, "water");

            //Temp
            tmp.temp = parseFloatValue(textStream, number, "temperature");
        }
        catch (const TParseException& err)
        {
            emit errorOccurred(QString::fromStdString(err.what()));

            textStream.readLine();

            ok = false;

            continue;
        }

        tmp.dateTime = QDateTime::currentDateTime();
        textStream.readLine();

        //проверям полученные значения
        if (!checkMeasument(number, tmp))
        {
            ok = false;

            continue;
        }
        _tanksMeasuments.insert(number, tmp);   
    }

    if (!ok)
    {
        Common::writeLogFile("PARSE ERR", QString("Data from level gauge:\n%1").arg(data));
    }
}

void TLS4::parseTanksEnabled(const QByteArray& data)
{
    QTextStream textStream(data);
    //Пропускаем 6 строчек
    skipLine(textStream, 6);

    while (!textStream.atEnd())
    {
        uint number;
        textStream >> number;
        if ((number < 1) || (number > MAX_TANK_NUMBER))
        {
            emit errorOccurred("parseTanksEnabled: Invalid tank number. Number:" + QString::number(number) + " Value ignored.");

            textStream.readLine();

            continue;
        }

        //считыввем продукт
        QString product;
        textStream >> product;
        //считываем вкл или выкл резервуар
        QString enabled;
        textStream >> enabled;
        _tanksConfigs[number].enabled = ((enabled == "BK") || (enabled == "ON") || (enabled == "Р'Р?"));
        if (_tanksConfigs[number].enabled)
        {
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

    while (!textStream.atEnd())
    {
        uint number;
        textStream >> number;
        if ((number < 1) || (number > MAX_TANK_NUMBER))
        {
            emit errorOccurred("parseTanksDiametr: Invalid tank number. Number:" + QString::number(number) + " Tank ignored.");

            textStream.readLine();

            continue;
        }
        //игнорируем название продукта
        QString ignoreTmp;
        textStream >> ignoreTmp;
        //считываем диаметр
        qint16 tmp;
        textStream >> tmp; //в случае ошибки tmp будет = 0
        if (((tmp < 10) || (tmp > 20000))  && (_tanksConfigs.contains(number) && _tanksConfigs[number].enabled))
        {
            emit errorOccurred("parseTanksDiametr: Tank:" + QString::number(number) + " Invalid value received. Diametr:" + QString::number(tmp) + " Value ignored.");

            textStream.readLine();

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

    while (!textStream.atEnd())
    {
        uint number;
        textStream >> number;
        if ((number < 1) || (number > MAX_TANK_NUMBER))
        {
            emit errorOccurred("parseTanksVolume: Invalid tank number. Number:" + QString::number(number) + " Tank ignored.");

            textStream.readLine();

            continue;
        }
        //игнорируем название продукта
        QString ignoreTmp;
        textStream >> ignoreTmp;

        qint32 tmp;
        textStream >> tmp;
        if (((tmp < 10) || (tmp > 10000000)) && (_tanksConfigs.contains(number) && _tanksConfigs[number].enabled))
        {
            emit errorOccurred("parseTanksVolume: Tank:" + QString::number(number) + " Invalid value received. Volume:" + QString::number(tmp) + " Value ignored.");

            textStream.readLine();

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

    while (!textStream.atEnd())
    {
        uint number;
        textStream >> number;
        if ((number < 1) || (number > MAX_TANK_NUMBER))
        {
            emit errorOccurred("parseTanksTilt: Invalid tank number. Number:" + QString::number(number) + " Tank ignored.");

            textStream.readLine();

            continue;
        }
        //игнорируем название продукта
        QString ignoreTmp;
        textStream >> ignoreTmp;

        float tmp;
        textStream >> tmp;
        if (((tmp < -180.0) || (tmp > 180.0)) && (_tanksConfigs.contains(number) && _tanksConfigs[number].enabled))
        {
            emit errorOccurred("parseTanksTilte: Tank:" + QString::number(number) + " Invalid value received. Volume:" + QString::number(tmp) + " Value ignored.");

            textStream.readLine();

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

    while (!textStream.atEnd())
    {
        uint number;
        textStream >> number;
        if ((number < 1) || (number > MAX_TANK_NUMBER))
        {
            emit errorOccurred("parseTanksTCCoef: Invalid tank number. Number:" + QString::number(number) + " Tank ignored.");

            textStream.readLine();

            continue;
        }
        //игнорируем название продукта
        QString ignoreTmp;
        textStream >> ignoreTmp;

        float tmp;
        textStream >> tmp;
        if (((tmp < -100) || (tmp > 100)) && (_tanksConfigs.contains(number) && _tanksConfigs[number].enabled))
        {
            emit errorOccurred("parseTanksTCCoef: Tank:" + QString::number(number) + " Invalid value received. Volume:" + QString::number(tmp) + " Value ignored.");

            textStream.readLine();

            continue;
        }
        _tanksConfigs[number].TCCoef = tmp;

        textStream.readLine();
    }
}

void TLS4::skipLine(QTextStream &stream, const int count)
{
    for (int i = 0; i < count; ++i )
    {
        stream.readLine();
    }
}

void TLS4::parseTanksOffset(const QByteArray &data)
{
    QTextStream textStream(data);
    //Пропускаем 6 строчек
    skipLine(textStream, 6);

    while (!textStream.atEnd())
    {
        uint number;
        textStream >> number;
        if ((number < 1) || (number > MAX_TANK_NUMBER))
        {
            emit errorOccurred("parseTankOffset: Invalid tank number. Number:" + QString::number(number) + " Tank ignored.");

            textStream.readLine();

            continue;
        }

        qint16 tmp;
        textStream >> tmp;
        if (((tmp < -100) || (tmp > 1000)) && (_tanksConfigs.contains(number) && _tanksConfigs[number].enabled))
        {
            emit errorOccurred("parseTankOffset: Tank:" + QString::number(number) + " Invalid value received. Volume:" + QString::number(tmp) + " Value ignored.");

            textStream.readLine();

            continue;
        }
        _tanksConfigs[number].offset = tmp;

        textStream.readLine();
    }
}

void TLS4::parseAnswer(QByteArray &data)
{
    writeDebugLogFile("LG answer:", QString(data));

    if (data.length() < 3)
    {
        emit errorOccurred("Data packet is too short");

        return;
    }

    if (data.left(8) == "9999FF1B")
    {
        emit errorOccurred("Level gauge return: Undefine command");

        return;
    }

    data.remove(0, 2);
    data.truncate(data.length() - 4);

    QString cmd(data.left(6));

    if (cmd == "I21400") //показания уровнемера
    {
        parseTanksMeasument(data);
        //данные готовы к отправке
        if (!_tanksMeasuments.isEmpty())
        {
            emit getTanksMeasument(_tanksMeasuments);
            _tanksMeasuments.clear();
        }
    }
    else if (cmd == "I60100")
    {
        parseTanksEnabled(data);
    }
    else if (cmd == "I60700")
    {
        parseTanksDiametr(data);
    }
    else if (cmd == "I60400")
    {
        parseTanksVolume(data);
    }
    else if (cmd == "I60800")
    {
        parseTanksTilt(data);
    }
    else if (cmd == "I60900")
    {
        parseTanksTCCoef(data);
    }
    else if (cmd == "I60C00") //эта команда отправляется последней при получении геометри резервуара
    {
        parseTanksOffset(data);
        //данные готовы к отправке
        if (!_tanksConfigs.isEmpty())
        {
            //записываем дату измерения
            for (const auto& tankNumber : _tanksConfigs.keys())
            {
                _tanksConfigs[tankNumber].dateTime = QDateTime::currentDateTime();
            }
            emit getTanksConfig(_tanksConfigs);
            _tanksConfigs.clear();
        }
    }
    else
    {
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
    if (_socket != nullptr)
    {
        return;
    }
    //если пришла пустая команда - запускаем отрпавку данных
    if (cmd.isEmpty()) {
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
    else
    {
        _cmdQueue.enqueue(char(1) + cmd + char(3));
    }
}

void TLS4::sendNextCmd()
{
    Q_ASSERT(_socket != nullptr);

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
        _socket->write(cmd);

        writeDebugLogFile("LG request:", QString(cmd));
    }
}

void TLS4::watchDocTimeout()
{
    Q_ASSERT(_socket != nullptr);

    emit errorOccurred("Connection timeout.");

    if (_socket != nullptr)
    {
        _socket->disconnectFromHost();
    }
}

void TLS4::transferReset()
{
    Q_ASSERT(_socket != nullptr);
    Q_ASSERT(_watchDoc != nullptr);

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
            _socket->abort();
           //тут может второй раз прилететь событие disconnect()
        }

        _socket->deleteLater();
        _socket = nullptr;
    }

    _readBuffer.clear();
}

void TLS4::getData()
{
    //раз в 100 тактов запрашиваем конфигурацию резервуаров
    if (((tick % 100) == 0) || (tick == 0))
    {
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
    _readBuffer += _socket->readAll();

    if (_readBuffer.size() > 0)
    {
        qsizetype posEXT = _readBuffer.indexOf(char(3)); //пришел символ 3 - конец пакета
        if (posEXT != -1)
        {
            _readBuffer.remove(posEXT, 1); //удаляем <EXT> в конце
            _readBuffer.remove(0, 1); //Удаляем <SOH>  в начале
            parseAnswer(_readBuffer);

            //очищаем буфер
            _readBuffer.clear();

            //отправляем слудующую
            sendNextCmd();
        }
    }
}

void TLS4::disconnentedSocket()
{
    transferReset();
}
