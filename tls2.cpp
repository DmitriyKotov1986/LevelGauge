#include "tls2.h"

//Qt
#include <QCoreApplication>

//My
#include "Common/common.h"

using namespace LevelGauge;
using namespace Common;

TLS2::TLS2(QObject* parent)
    : TLevelGauge(parent)
    , _cnf(TConfig::config())
{
    Q_CHECK_PTR(_cnf);
}

TLS2::~TLS2()
{
    Q_CHECK_PTR(_watchDoc);

    if (_socket != nullptr)
    {
        _socket->abort();

        delete _socket;
    }

    delete _watchDoc;
    delete _getDataTimer;
}

void TLS2::start()
{
    Q_ASSERT(_socket == nullptr);
    Q_ASSERT(_watchDoc == nullptr);

    _getDataTimer = new QTimer();
    QObject::connect(_getDataTimer, SIGNAL(timeout()), SLOT(getData()));
    _getDataTimer->start(_cnf->sys_Interval());

    //WatchDoc
    _watchDoc = new QTimer();
    QObject::connect(_watchDoc, SIGNAL(timeout()), SLOT(watchDocTimeout()));
    _watchDoc->setSingleShot(true);

    getData();
}

void TLS2::connentedSocket()
{
    sendNextCmd();
    //ответ придет в readData()
}

void TLS2::disconnentedSocket()
{
    transferReset();
}

void TLS2::parseTanksMeasument(const QByteArray& data)
{
    Q_ASSERT(_tanksMeasuments.isEmpty());

/*
9999FF1B^AI21400

I21400
24.12.23 21:37






ЗАПАСЫ ПО МАССЕ В РЕЗ

РЕЗ  ПРОДУКТ                ОБЪЕМ        МАС     ПЛОТ-ТЬ     УР.  ВОДА    ТЕМП
  1  Au-92/1                18351   13619.71       742.2  1925.9   0.0    10.0
  2  Au-95                   5773    4311.06       746.8   705.8   0.0     8.9
  3  Au-92/2                 4761    3527.18       740.9   599.1   0.0     9.8
  4  Au-92/3                15492   11515.11       743.3  1571.1   0.0     6.6
  5  DT                      8421    7134.62       847.3  1606.0   0.0    11.1
*/

    QTextStream textStream(data);
    //Пропускаем 11 строчек
    skipLine(textStream, 11);
    //парсим таблицу с данными
    bool ok = true;
    while (!textStream.atEnd())
    {
        TLevelGauge::TTankMeasument tmp;
        uint number = 0;
        textStream >> number;
        QString IgnoreStr;
        textStream >> IgnoreStr; //здесь записан имя продукта, но мы ее игнорируем
        textStream >> tmp.volume;
        float currValue;
        textStream >> currValue;
        tmp.mass = static_cast<int>(currValue);
        textStream >> tmp.density;
        textStream >> currValue;
        tmp.height = static_cast<int>(currValue);
        textStream >> tmp.water;
        textStream >> tmp.temp;

        tmp.dateTime = QDateTime::currentDateTime();

        //проверям полученные значения
        if (!checkMeasument(number, tmp))
        {
            textStream.readLine();

            ok = false;

            continue;
        }

        _tanksMeasuments.insert(number, tmp);

        textStream.readLine();
    }

    if (!ok)
    {
        Common::writeDebugLogFile("PARSE ERR", QString("Data from level gauge:\n%1").arg(data));
    }
}

void TLS2::skipLine(QTextStream &stream, const int count)
{
    for (int i = 0; i < count; ++i)
    {
        stream.readLine();
    }
}

void TLS2::parseAnswer(QByteArray &data)
{
    Common::writeDebugLogFile("LG answer:", QString(data));

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
    else
    {
        emit errorOccurred("Undefine function in answer. Level gauge will be reboot. Data: " + data);
        sendCmd("S00100"); //перезагружаем уровнемер, если что-то пошло не так
        sendCmd("");
    }
}

void TLS2::upDateTanksMeasuments()
{
    sendCmd("I21400"); //текущие измерения
}

void TLS2::sendCmd(const QByteArray &cmd)
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
    else
    {
        _cmdQueue.enqueue(char(1) + cmd + char(3));
    }
}

void TLS2::sendNextCmd()
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

        Common::writeDebugLogFile("LG request:", QString(cmd));
    }
}

void TLS2::transferReset()
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
           // _socket->waitForDisconnected(5000);
           //тут может второй раз прилететь событие disconnect()
        }

        _socket->deleteLater();
        _socket = nullptr;
    }

    _readBuffer.clear();
}

void TLS2::getData()
{
    upDateTanksMeasuments();
    sendCmd("");

    ++tick;
}

void TLS2::watchDocTimeout()
{
    Q_ASSERT(_socket != nullptr);

    emit errorOccurred("Connection timeout");

    if (_socket != nullptr)
    {
        _socket->disconnectFromHost();
    }
}

void TLS2::errorOccurredSocket(QAbstractSocket::SocketError)
{
    emit errorOccurred(QString("Socket error. Code: %1. Msg: %2").arg(_socket->error()).arg(_socket->errorString()));
    transferReset();
}

void TLS2::readyReadSocket()
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
