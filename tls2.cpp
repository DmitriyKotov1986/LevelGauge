#include "tls2.h"

//Qt
#include <QCoreApplication>
#include <QFile>

using namespace LevelGauge;

TLS2::TLS2(TConfig* cnf, QObject* parent) :
    TLevelGauge(parent),
    _cnf(cnf)
{
    Q_ASSERT(cnf != nullptr);
}

TLS2::~TLS2()
{
    Q_ASSERT(_socket != nullptr);

    if (_socket != nullptr) {
        if (_socket->isOpen()) {
           _socket->disconnectFromHost();
        }
        _socket->deleteLater();
        _socket = nullptr;
    }
}

void TLS2::start()
{
    Q_ASSERT(_socket == nullptr);

    _socket = new QTcpSocket;
    QObject::connect(_socket, SIGNAL(readyRead()), SLOT(readyReadSocket()));
    QObject::connect(_socket, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), SLOT(errorOccurredSocket(QAbstractSocket::SocketError)));
}

void TLS2::readyReadSocket() //пришли данные в сокет
{

}

void TLS2::errorOccurredSocket(QAbstractSocket::SocketError) //возникла ошибка подключения
{

}

void TLS2::parseTanksMeasument(const QByteArray& data)
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
        textStream >> tmp.mass;
        textStream >> tmp.density;
        textStream >> tmp.TKCorrect;
        textStream >> tmp.height;
        textStream >> tmp.water;
        textStream >> tmp.temp;

        tmp.dateTime = QDateTime::currentDateTime();
        textStream.readLine();

        #ifdef QT_DEBUG
            qDebug() << "Num=" << number <<
                        "Volume=" << tmp.volume <<
                        "Mass=" << tmp.mass <<
                        "Density=" << tmp.density <<
                        "TKCorrect=" << tmp.TKCorrect <<
                        "Height=" << tmp.height <<
                        "Water=" << tmp.water <<
                        "Temp=" << tmp.temp;
        #endif
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

void TLS2::parseTanksEnabled(const QByteArray& data)
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
        textStream >> _tanksConfigs[number - 1].product;
        //считываем вкл или выкл резервуар
        QString tmp;
        textStream >> tmp;
        _tanksConfigs[number - 1].enabled = ((tmp == "BK") || (tmp == "ON") || (tmp == "Р'Р?"));

        textStream.readLine();
    }
}

void TLS2::parseTanksDiametr(const QByteArray& data)
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
        if ((tmp < 10) || (tmp > 20000)) {
            emit errorOccurred("parseTanksDiametr: Tank:" + QString::number(number) + " Invalid value received. Diaaetr:" + QString::number(tmp) + " Value ignored.");
            continue;
        }

        _tanksConfigs[number].diametr = tmp;


        textStream.readLine();
    }
}

void TLS2::parseTanksVolume(const QByteArray& data)
{
    QTextStream textStream(data);
    //Пропускаем 6 строчек
    for (uint8_t i = 0; i <= 5; ++i ) {
        textStream.readLine();
    }

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
        if ((tmp < 10) || (tmp > 10000000)) {
            emit errorOccurred("parseTanksVolume: Tank:" + QString::number(number) + " Invalid value received. Volume:" + QString::number(tmp) + " Value ignored.");
            continue;
        }

        _tanksConfigs[number].volume = tmp;

        textStream.readLine();
     }
}

void TLS2::parseTanksTilt(const QByteArray& data)
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
        if ((tmp < -180.0) || (tmp > 180.0)) {
            emit errorOccurred("parseTanksTilte: Tank:" + QString::number(number) + " Invalid value received. Volume:" + QString::number(tmp) + " Value ignored.");
            continue;
        }
        _tanksConfigs[number].tilt = tmp;

        textStream.readLine();
    }
}

void TLS2::parseTanksTCCoef(const QByteArray& data)
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
        if ((tmp < -100) || (tmp > 100)) {
            emit errorOccurred("parseTanksTCCoef: Tank:" + QString::number(number) + " Invalid value received. Volume:" + QString::number(tmp) + " Value ignored.");
            continue;
        }
        _tanksConfigs[number].TCCoef = tmp;

        textStream.readLine();
    }
}

void TLS2::skipLine(QTextStream &stream, const int count)
{
    for (int i = 0; i < count; ++i ) stream.readLine();
}

void TLS2::parseTanksOffset(const QByteArray &data)
{
    QTextStream textStream(data);
    //Пропускаем 5 строчек
    skipLine(textStream, 5);
    while (!textStream.atEnd())  {
        uint number;
        textStream >> number;
        if ((number < 1) || (number > 6)) {
            emit errorOccurred("parseTanksTCCoef: Invalid tank number. Number:" + QString::number(number) + " Tank ignored.");
            continue;
        }

        qint16 tmp;
        textStream >> tmp;
        if ((tmp < -100) || (tmp > 1000)) {
            emit errorOccurred("parseTankPffset: Tank:" + QString::number(number) + " Invalid value received. Volume:" + QString::number(tmp) + " Value ignored.");
            continue;
        }
        _tanksConfigs[number].offset = tmp;

        textStream.readLine();
    }
}

void TLS2::parseAnswer(QByteArray &data)
{
    #ifdef QT_DEBUG
        QFile file(LevelGauge::LOG_FILE_NAME);
        if (file.open(QIODevice::Append)) {
            file.write(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz").toUtf8() + " LG answer:\r\n");
            file.write(data);
            file.close();
        }
    #endif

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
            emit getTanksMeasument(_tanksMeasuments);
            _tanksMeasuments.clear();
        }
    }
    else {
        emit errorOccurred("Undefine function in answer. Level gauge will be reboot. Data: " + data);
        sendCmd("S00100"); //перезагружаем уровнемер, если что-то пошло не так
        sendCmd("");
    }
}

void TLS2::upDateTanksConfigs()
{
    sendCmd("I60100"); //продукт и вкл
    sendCmd("I60700"); //диаметр
    sendCmd("I60400"); //объем
    sendCmd("I60800"); //наклон
    sendCmd("I60900"); //Температурный коэф
    sendCmd("I60C00"); //смещение по высоте
    sendCmd("");
}

void TLS2::upDateTanksMeasuments()
{
    sendCmd("I21400"); //текущие измерения
    sendCmd("");
}





/*#include "tlevelgauge.h"
#include <QTextStream>
#include <QtCore5Compat/QTextCodec>
#include <QHostAddress>
#include <QFile>
#include "TLevelGaugeMonitoring.h"

TLevelGauge::TLevelGauge(QSettings & Config, QObject *parent)
    : QTcpSocket(parent)
{
    QObject::connect(this, SIGNAL(connected()), this, SLOT(onConnected()));
    QObject::connect(this, SIGNAL(readyRead()), this, SLOT(onReadyRead()));
    QObject::connect(this, SIGNAL(errorOccurred(QAbstractSocket::SocketError)), this, SLOT(onErrorOccurred(QAbstractSocket::SocketError)));
    QObject::connect(this, SIGNAL(disconnected()), this, SLOT(onDisconnected()));

    Config.beginGroup("LEVELGAUDE");
    LevelGaugeAdr = Config.value("Host", "localhost").toString();
    LevelGaugePort = Config.value("Port", "10001").toUInt();
    TLSModel = Config.value("TLS", "2").toUInt();
   // qDebug() << TLSModel;
    if (!((TLSModel == 4)||(TLSModel == 2))) {
        qDebug() << "Unsupport model of TLS. Use model TLS 2";
        TLSModel = 2;
    }
    Config.endGroup();

    WaitAnswerTimer.setInterval(10000);
    WaitAnswerTimer.setSingleShot(true);
    QObject::connect(&WaitAnswerTimer, SIGNAL(timeout()), this, SLOT(onTransferTimeout()));
}

TLevelGauge::~TLevelGauge()
{
    close();
}

void TLevelGauge::SendCmd(const QByteArray &cmd)
{

    CmdQueue.enqueue(char(1) + cmd + char(3));
 //   qDebug() << cmd;
  //  emit SendLogMsg(TLevelGaugeMonitoring::CODE_ERROR, "Transfering:" + QString(Transfering ? "true" : "false"));
    if (!Transfering) {
        Transfering = true;
        connectToHost(LevelGaugeAdr, LevelGaugePort, QIODeviceBase::ReadWrite, QAbstractSocket::IPv4Protocol);
        WaitAnswerTimer.start();
    }
}

void TLevelGauge::SendNextCmd()
{
//    qDebug() << "Queue size = " << CmdQueue.size() ;
    if (CmdQueue.isEmpty()) return; //все команды отправлены - выходим


    QByteArray tmp = CmdQueue.dequeue();
  //  emit SendLogMsg(TLevelGaugeMonitoring::CODE_INFORMATION, "Send cmd:" + QString::fromUtf8(tmp));

    if (isOpen()) {
        write(tmp);
        if (tmp == (char(1) + QByteArray("S00100") + char(3))) { //если отправлена команда на сброс - то отключаемся
            waitForBytesWritten();
            TransferReset();
        }
    }


    else emit SendLogMsg(TLevelGaugeMonitoring::CODE_ERROR, "Try to send command to level gauge when the connection is not established");
}

void TLevelGauge::TransferReset()
{
    Transfering = false;
    close();
    CmdQueue.clear();
    WaitAnswerTimer.stop();
    ++TransferAttempt;
  //  emit SendLogMsg(TLevelGaugeMonitoring::CODE_INFORMATION, "Transfer reset.");
}

void TLevelGauge::UpDateTanksConfig()
{
    SendCmd("I60100"); //продукт и вкл
    SendCmd("I60700"); //диаметр
    SendCmd("I60400"); //объем
    SendCmd("I60800"); //наклон
    SendCmd("I60900"); //Температурный коэф
    SendCmd("I60C00"); //смещение по высоте
}

void TLevelGauge::UpDateTanksMeasument()
{
    SendCmd("I21400"); //текущие измерения
}

TLevelGauge::TLevelGaugeData &TLevelGauge::GetTanksInfo()
{
    return LevelGaugeData;
}

void TLevelGauge::ParseTanksMeasument(const QByteArray &data)
{
    QTextStream TextStream(data);
    //Пропускаем 11 строчек
    for (uint8_t i = 0; i <= 10; ++i ) TextStream.readLine();
    while (!TextStream.atEnd())  {
        TTanksMeasument tmp;
        if (TLSModel == 2) {
            TextStream >> tmp.Number;
            QString IgnoreStr;
            TextStream >> IgnoreStr; //здесь записан имя продукта, но мы ее игнорируем
            TextStream >> tmp.Volume;
            TextStream >> tmp.Mass;
            TextStream >> tmp.Density;
            TextStream >> tmp.TKCorrect;
            TextStream >> tmp.Height;
            TextStream >> tmp.Water;
            TextStream >> tmp.Temp;
        }
        if (TLSModel == 4) {
            TextStream >> tmp.Number;
            QString IgnoreStr;
            TextStream >> IgnoreStr; //здесь записан имя продукта, но мы ее игнорируем
            TextStream >> tmp.Volume;
            //Mass
            float CurrValue;
            TextStream >> CurrValue;
            tmp.Mass = static_cast<int>(CurrValue);
            //density
            TextStream >> tmp.Density;
            tmp.TKCorrect = 0.0;
            //height
            TextStream >> CurrValue;
            tmp.Height = static_cast<int>(CurrValue);

            TextStream >> tmp.Water;
            TextStream >> tmp.Temp;
        }

        tmp.DateTime = QDateTime::currentDateTime();
        TextStream.readLine();

  //     qDebug() << "Num=" << tmp.Number <<
  //      "Volume=" << tmp.Volume <<
 //       "Mass=" << tmp.Mass <<
//       "Density=" << tmp.Density <<
  //      "TKCorrect=" << tmp.TKCorrect <<
//        "Height=" << tmp.Height <<
 //       "Water=" << tmp.Water <<
 //       "Temp=" << tmp.Temp;

        if (tmp.Volume < 10) {
            emit SendLogMsg(TLevelGaugeMonitoring::CODE_ERROR, "Tank:" + QString::number(tmp.Number) + " Invalid value received. Volume:" + QString::number(tmp.Volume) + " Tank ignored.");
            continue;
        }
        if (tmp.Mass < 10) {
            emit SendLogMsg(TLevelGaugeMonitoring::CODE_ERROR, "Tank:" + QString::number(tmp.Number) + " Invalid value received. Mass:" + QString::number(tmp.Mass) + " Tank ignored.");
            continue;
        }
        if (tmp.Density < 500.0) {
            emit SendLogMsg(TLevelGaugeMonitoring::CODE_ERROR, "Tank:" + QString::number(tmp.Number) + " Invalid value received. Density:" + QString::number(tmp.Density) + " Tank ignored.");
            continue;
        }
        if (tmp.Height < 10) {
            emit SendLogMsg(TLevelGaugeMonitoring::CODE_ERROR, "Tank:" + QString::number(tmp.Number) + " Invalid value received. Height:" + QString::number(tmp.Height) + " Tank ignored.");
            continue;
        }
        if ((tmp.Temp < -60.0)||(tmp.Temp > 60.0)) {
            emit SendLogMsg(TLevelGaugeMonitoring::CODE_ERROR, "Tank:" + QString::number(tmp.Number) + " Invalid value received. Temp:" + QString::number(tmp.Temp) + " Tank ignored.");
            continue;
        }
        LevelGaugeData.Measument.enqueue(tmp);
    }

 //  while (!LevelGaugeData.Measument.isEmpty()) {
 //       TTanksMeasument TmpTanksMeasument = LevelGaugeData.Measument.dequeue();
 //       qDebug() << "Num=" << TmpTanksMeasument.Number <<
//        "Volume=" << TmpTanksMeasument.Volume <<
//        "Mass=" << TmpTanksMeasument.Mass <<
///        "Density=" << TmpTanksMeasument.Density <<
//        "TKCorrect=" << TmpTanksMeasument.TKCorrect <<
 //       "Height=" << TmpTanksMeasument.Height <<
 //       "Water=" << TmpTanksMeasument.Water <<
 //       "Temp=" << TmpTanksMeasument.Temp;
//    }
}

void TLevelGauge::ParseTanksEnabled(const QByteArray &data)
{
    QTextStream TextStream(data);
    //Пропускаем 6 строчек
    for (uint8_t i = 0; i <= 5; ++i ) TextStream.readLine();

    while (!TextStream.atEnd())  {
        uint TankNumber;
        TextStream >> TankNumber;
        if (LevelGaugeData.TanksConfig.size() < TankNumber) LevelGaugeData.TanksConfig.resize(TankNumber);
        TextStream >> LevelGaugeData.TanksConfig[TankNumber - 1].Product;
        QString tmp;
        TextStream >> tmp;
        bool NewValue = ((tmp == "BK") || (tmp == "ON") || (tmp == "Р'Р?"));
    //    qDebug() << TankNumber << " " << tmp << " " << NewValue;
        if (LevelGaugeData.TanksConfig[TankNumber - 1].Enabled != NewValue) {
            //qDebug() << "Enabled";
            LevelGaugeData.TanksConfig[TankNumber - 1].NewData = true;
            LevelGaugeData.TanksConfig[TankNumber - 1].Enabled = NewValue;
            LevelGaugeData.TanksConfig[TankNumber - 1].DateTime = QDateTime::currentDateTime();
        }
        TextStream.readLine();
    }
}

void TLevelGauge::ParseTanksDiametr(const QByteArray &data)
{
    QTextStream TextStream(data);
    //Пропускаем 6 строчек
    for (uint8_t i = 0; i <= 5; ++i ) TextStream.readLine();

    while (!TextStream.atEnd())  {
        uint TankNumber;
        TextStream >> TankNumber;
        if (LevelGaugeData.TanksConfig.size() < TankNumber) LevelGaugeData.TanksConfig.resize(TankNumber);
        TextStream >> LevelGaugeData.TanksConfig[TankNumber - 1].Product;
        //QString IgnoreStr;
        //TextStream >> IgnoreStr; //здесь записан имя продукта, но мы ее игнорируем
        float NewValue;
        TextStream >> NewValue;
        if (LevelGaugeData.TanksConfig[TankNumber - 1].Diametr != NewValue) {
         //   qDebug() << "Diametr" << NewValue;
            LevelGaugeData.TanksConfig[TankNumber - 1].NewData = true;
            LevelGaugeData.TanksConfig[TankNumber - 1].Diametr = NewValue;
            LevelGaugeData.TanksConfig[TankNumber - 1].DateTime = QDateTime::currentDateTime();
        }
        TextStream.readLine();
    }
}

void TLevelGauge::ParseTanksVolume(const QByteArray &data)
{
    QTextStream TextStream(data);
    //Пропускаем 6 строчек
    for (uint8_t i = 0; i <= 5; ++i ) TextStream.readLine();

    while (!TextStream.atEnd())  {
        uint TankNumber;
        TextStream >> TankNumber;
        if (LevelGaugeData.TanksConfig.size() < TankNumber) LevelGaugeData.TanksConfig.resize(TankNumber);
        TextStream >> LevelGaugeData.TanksConfig[TankNumber - 1].Product;
        //QString IgnoreStr;
        //TextStream >> IgnoreStr; //здесь записан имя продукта, но мы ее игнорируем
        float NewValue;
        TextStream >> NewValue;
        if (LevelGaugeData.TanksConfig[TankNumber - 1].Volume != NewValue) {
            //qDebug() << "Volume";
            LevelGaugeData.TanksConfig[TankNumber - 1].NewData = true;
            LevelGaugeData.TanksConfig[TankNumber - 1].Volume = NewValue;
            LevelGaugeData.TanksConfig[TankNumber - 1].DateTime = QDateTime::currentDateTime();
        }
        TextStream.readLine();
     }
}

void TLevelGauge::ParseTanksTilt(const QByteArray &data)
{
    QTextStream TextStream(data);
    //Пропускаем 6 строчек
    for (uint8_t i = 0; i <= 5; ++i ) TextStream.readLine();

    while (!TextStream.atEnd())  {
        uint TankNumber;
        TextStream >> TankNumber;
        if (LevelGaugeData.TanksConfig.size() < TankNumber) LevelGaugeData.TanksConfig.resize(TankNumber);
        TextStream >> LevelGaugeData.TanksConfig[TankNumber - 1].Product;
        //QString IgnoreStr;
        //TextStream >> IgnoreStr; //здесь записан имя продукта, но мы ее игнорируем
        float NewValue;
        TextStream >> NewValue;
        if (LevelGaugeData.TanksConfig[TankNumber - 1].Tilt != NewValue) {
            //qDebug() << "Tilt";
            LevelGaugeData.TanksConfig[TankNumber - 1].NewData = true;
            LevelGaugeData.TanksConfig[TankNumber - 1].Tilt = NewValue;
            LevelGaugeData.TanksConfig[TankNumber - 1].DateTime = QDateTime::currentDateTime();
        }
        TextStream.readLine();
    }
}

void TLevelGauge::ParseTanksTCCoef(const QByteArray &data)
{
    QTextStream TextStream(data);
    //Пропускаем 6 строчек
    for (uint8_t i = 0; i <= 5; ++i ) TextStream.readLine();

    while (!TextStream.atEnd())  {
        uint TankNumber;
        TextStream >> TankNumber;
        if (LevelGaugeData.TanksConfig.size() < TankNumber) LevelGaugeData.TanksConfig.resize(TankNumber);
        TextStream >> LevelGaugeData.TanksConfig[TankNumber - 1].Product;
        //QString IgnoreStr;
        //TextStream >> IgnoreStr; //здесь записан имя продукта, но мы ее игнорируем
        float NewValue;
        TextStream >> NewValue;
        if (LevelGaugeData.TanksConfig[TankNumber - 1].TCCoef != NewValue) {
            //qDebug() << "TCCoef";
            LevelGaugeData.TanksConfig[TankNumber - 1].NewData = true;
            LevelGaugeData.TanksConfig[TankNumber - 1].TCCoef = NewValue;
            LevelGaugeData.TanksConfig[TankNumber - 1].DateTime = QDateTime::currentDateTime();
        }
        TextStream.readLine();
    }
}

void TLevelGauge::ParseTanksOffset(const QByteArray &data)
{
    QTextStream TextStream(data);
    //qDebug() << data;
    //Пропускаем 5 строчек
    if (TLSModel == 2) {
        for (uint8_t i = 0; i <= 4; ++i ) TextStream.readLine();
    }
    if (TLSModel == 4) {
        for (uint8_t i = 0; i <= 5; ++i ) TextStream.readLine();
    }
    while (!TextStream.atEnd())  {
        uint TankNumber;
        TextStream >> TankNumber;
       // qDebug() << "TankNumber " << TankNumber;
        if (LevelGaugeData.TanksConfig.size() < TankNumber) LevelGaugeData.TanksConfig.resize(TankNumber);
        TextStream >> LevelGaugeData.TanksConfig[TankNumber - 1].Product;
      //  qDebug() << "Product " << LevelGaugeData.TanksConfig[TankNumber - 1].Product;
       // TextStream.read(20);
        float NewValue;
        TextStream >> NewValue;
      //  qDebug() << "Offset " << NewValue;
        if (LevelGaugeData.TanksConfig[TankNumber - 1].Offset != NewValue) {
            LevelGaugeData.TanksConfig[TankNumber - 1].NewData = true;
            LevelGaugeData.TanksConfig[TankNumber - 1].Offset = NewValue;
            LevelGaugeData.TanksConfig[TankNumber - 1].DateTime = QDateTime::currentDateTime();
        }
        TextStream.readLine();
    }
}

void TLevelGauge::ParseAnswer(QByteArray &data)
{
//    emit SendLogMsg(TLevelGaugeMonitoring::CODE_INFORMATION, "Get answer:" + QString::fromUtf8(data));
//    QFile File("LevelGauge.log");
//    if (File.open(QIODevice::Append)) {
//        File.write(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz").toUtf8() + ":\r\n");
//        File.write(data);
//        File.close();
//    }


    if (data.length() < 3) {
        emit SendLogMsg(TLevelGaugeMonitoring::CODE_ERROR, "Data packet is too short");
        return;
    }

    if (data.left(8) == "9999FF1B") {
        emit SendLogMsg(TLevelGaugeMonitoring::CODE_ERROR, "Level gauge return: Undefine command");
        return;
    }

    data.remove(0, 2);
    data.truncate(data.length() - 4);

    QString cmd(data.left(6));

    if (cmd == "I21400") {
        ParseTanksMeasument(data);
    }
    else if (cmd == "I60100") {
        ParseTanksEnabled(data);
    }
    else if (cmd == "I60700") {
        ParseTanksDiametr(data);
    }
    else if (cmd == "I60400") {
        ParseTanksVolume(data);
    }
    else if (cmd == "I60800") {
        ParseTanksTilt(data);
    }
    else if (cmd == "I60900") {
        ParseTanksTCCoef(data);
    }
    else if (cmd == "I60C00") {
        ParseTanksOffset(data);
    }
    else {
        emit SendLogMsg(TLevelGaugeMonitoring::CODE_ERROR, "Undefine function in answer. Level gauge will be reboot. Data: " + data);
        SendCmd("S00100"); //перезагружаем уровнемер, если что-то пошло не так
    }
 //   qDebug() << cmd;

 _tanksConfigs[number - 1].dateTime - pfgjkyзаполни это поле
}

void TLevelGauge::onConnected()
{

 //   emit SendLogMsg(TLevelGaugeMonitoring::CODE_INFORMATION, "Level gauge is connected Host: " + peerName() + " Port: " + QString::number(peerPort()));
    SendNextCmd();
 //   UpDateTanksConfig();
 //   UpDateTanksMeasument();
 //   for (uint i = 0; i < LevelGaugeData.TanksConfig.size(); ++i) {
 //       qDebug() << i << " Enabled=" << LevelGaugeData.TanksConfig[i].Enabled <<
   //                      " Offset=" << LevelGaugeData.TanksConfig[i].Offset <<
//                         " TCCoef=" << LevelGaugeData.TanksConfig[i].TCCoef <<
//                        " Tilt=" << LevelGaugeData.TanksConfig[i].Tilt <<
//                         " Volume=" << LevelGaugeData.TanksConfig[i].Volume <<
//                         " Diametr=" << LevelGaugeData.TanksConfig[i].Diametr <<
 //                        " Product=" << LevelGaugeData.TanksConfig[i].Product;
 //   }
}

void TLevelGauge::onDisconnected()
{
//   emit SendLogMsg(TLevelGaugeMonitoring::CODE_INFORMATION, "Level gauge is diconnected");
}

void TLevelGauge::onErrorOccurred(QAbstractSocket::SocketError Err)
{
    TransferReset();
    emit SendLogMsg(TLevelGaugeMonitoring::CODE_ERROR, "Socket error: "  + errorString() + ". Attempt: " + QString::number(TransferAttempt) +" Host: " + LevelGaugeAdr + " Port: " + QString::number(LevelGaugePort));
    emit GetDataComplite();
}

void TLevelGauge::onReadyRead()
{
//    qDebug() << CmdQueue.size();
    ReadBuffer += readAll();

    if (ReadBuffer.size() > 0) {
        qsizetype PosEXT = ReadBuffer.indexOf(char(3));
        if (PosEXT != -1) {
            //qDebug() << "(!)Buffer: " << ReadBuffer;
            QTextCodec *codec = QTextCodec::codecForName("IBM 866");
            QByteArray tmp = codec->toUnicode(ReadBuffer.left(PosEXT)).toUtf8();
            ReadBuffer.remove(0, PosEXT + 1);
            tmp.remove(0, 1); //Удаляем <SOH>  в начале
            //qDebug() << "(!) In parser:" <<tmp;
            //qDebug() << "(!)Buffer after parser: " << ReadBuffer;
            WaitAnswerTimer.stop();
            ParseAnswer(tmp);
            if (!CmdQueue.isEmpty()) SendNextCmd();
            else {
                Transfering = false;
                close();
                TransferAttempt = 0;
                emit GetDataComplite();
            }
        }
    }
}

void TLevelGauge::onTransferTimeout()
{
    TransferReset();
    emit SendLogMsg(TLevelGaugeMonitoring::CODE_ERROR, "Data transmission timeout. Attempt: " + QString::number(TransferAttempt));
    emit GetDataComplite();
}

*/


