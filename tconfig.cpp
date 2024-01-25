#include <QSettings>
#include <QFileInfo>
#include <QDebug>
#include "tconfig.h"

using namespace LevelGauge;
using namespace Common;

//static
static TConfig* configPtr = nullptr;

TConfig* TConfig::config(const QString& configFileName)
{
    if (configPtr == nullptr)
    {
        configPtr = new TConfig(configFileName);
    }

    return configPtr;
};

void TConfig::deleteConfig()
{
    Q_CHECK_PTR(configPtr);

    delete configPtr;
    configPtr = nullptr;
}

TConfig::TConfig(const QString& configFileName) :
    _configFileName(configFileName)
{
    if (_configFileName.isEmpty())
    {
        _errorString = "Configuration file name cannot be empty";

        return;
    }
    if (!QFileInfo(_configFileName).exists())
    {
        _errorString = "Configuration file not exist. File name: " + _configFileName;

        return;
    }

    qDebug() << QString("%1 %2").arg(QTime::currentTime().toString(SIMPLY_TIME_FORMAT)).arg("Reading configuration from " +  _configFileName);

    QSettings ini(_configFileName, QSettings::IniFormat);

    QStringList groups = ini.childGroups();
    if (!groups.contains("DATABASE"))
    {
        _errorString = "Configuration file not contains [DATABASE] group";

        return;
    }
    if (!groups.contains("SYSTEM"))
    {
        _errorString = "Configuration file not contains [SYSTEM] group";

        return;
    }
    if (!groups.contains("SERVER"))
    {
        _errorString = "Configuration file not contains [SERVER] group";

        return;
    }
    if (!groups.contains("LEVELGAUGE"))
    {
        _errorString = "Configuration file not contains [LEVELGAUGE] group";

        return;
    }

    //Database
    ini.beginGroup("DATABASE");

    _db_ConnectionInfo.db_Driver = ini.value("Driver", "QODBC").toString();
    _db_ConnectionInfo.db_DBName = ini.value("DataBase", "SystemMonitorDB").toString();
    _db_ConnectionInfo.db_UserName = ini.value("UID", "").toString();
    _db_ConnectionInfo.db_Password = ini.value("PWD", "").toString();
    _db_ConnectionInfo.db_ConnectOptions = ini.value("ConnectionOprions", "").toString();
    _db_ConnectionInfo.db_Port = ini.value("Port", "").toUInt();
    _db_ConnectionInfo.db_Host = ini.value("Host", "localhost").toString();

    ini.endGroup();

    ini.beginGroup("SYSTEM");

    _sys_Interval = ini.value("Interval", "60000").toInt();
    _sys_DebugMode = ini.value("DebugMode", "0").toBool();

    ini.endGroup();

    ini.beginGroup("SERVER");
    _srv_UserName = ini.value("UID", "000").toString();
    _srv_Password = ini.value("PWD", "").toString();
    _srv_Host = ini.value("Host", "localhost").toString();
    _srv_Port = ini.value("Port", "").toUInt();
    _srv_MaxRecord = ini.value("MaxRecord", "100").toUInt();
    ini.endGroup();

    ini.beginGroup("LEVELGAUGE");
    _lg_Host = ini.value("HOST", "localhost").toString();
    _lg_Port = ini.value("Port", "10001").toUInt();
    _lg_TLS = ini.value("TLS", 2).toUInt();

    if ((_lg_TLS == 2) || (_lg_TLS == 4))
    {
        //Доп. параметры не нужны
    }
    else if ((_lg_TLS == 100) || (_lg_TLS == 101))
    {
        QStringList lgAddressList(ini.value("Addresses", "").toStringList());
        for (const auto& addressItem: lgAddressList)
        {
            bool ok;
            _lg_Address.push_back(addressItem.toUInt(&ok, 16));
            if (!ok)
            {
                _errorString = "[LEVELGAUGE]/Addresses: " + addressItem + " is not number";

                return;
            }
        }
    }
    else if (_lg_TLS == 200)
    {
        _lg_userName = ini.value("UserName", "fafnir").toString();
        _lg_password = ini.value("Password", "fafnir22766").toString();
    }
    else if (_lg_TLS == 201)
    {
        //Доп. параметры не нужны
    }
    else
    {
         _errorString = "[LEVELGAUGE]/TLS: undefite TLS type";

         return;
    }


    ini.endGroup();
}

bool TConfig::save()
{
    QSettings ini(_configFileName, QSettings::IniFormat);

    if (!ini.isWritable()) {
        _errorString = "Can not write configuration file " +  _configFileName;

        return false;
    }

    ini.beginGroup("SERVER");
    ini.endGroup();

    ini.sync();

    if (_sys_DebugMode) {
        qDebug() << QString("%1 %2").arg(QTime::currentTime().toString(SIMPLY_TIME_FORMAT)).arg("Save configuration to " +  _configFileName);
    }

    return true;
}

QString TConfig::errorString()
{
    auto res = _errorString;
    _errorString.clear();

    return res;
}

