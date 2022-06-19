#include "tls2.h"

using namespace LevelGauge;

TLS2::TLS2(TConfig* cnf, QObject* parent) :
    TLevelGauge(parent)
{

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


