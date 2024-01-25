#ifndef TCONFIG_H
#define TCONFIG_H

#include <QString>
#include <QFile>

#include "Common/Common.h"

namespace LevelGauge {

class TConfig
{
public:
    static TConfig* config(const QString& configFileName = "");
    static void deleteConfig();

private:
    explicit TConfig(const QString& configFileName);

public:
    bool save();

    //[DATABASE]
    const Common::DBConnectionInfo& db_ConnectionInfo() const { return _db_ConnectionInfo; }

    //[LEVELGAUGE]
    const QString& lg_Host() const { return _lg_Host; }
    quint16 lg_Port() const { return _lg_Port; }
    quint16 lg_TLS() const { return _lg_TLS; }
    const QList<uint8_t>& lg_Addresses() const { return _lg_Address; }
    const QString& lg_userName() const { return _lg_userName; }
    const QString& lg_password() const { return _lg_password; }

    //[SYSTEM]
    int sys_Interval() const { return _sys_Interval; }
    bool sys_DebugMode() const { return _sys_DebugMode; }

    //[SERVER]
    const QString& srv_Host() const { return _srv_Host; }
    quint16 srv_Port() const { return _srv_Port; }
    const QString& srv_UserName() const { return _srv_UserName; }
    const QString& srv_Password() const { return _srv_Password; }
    int srv_MaxRecord() const { return _srv_MaxRecord; }

    QString errorString();
    bool isError() const { return !_errorString.isEmpty(); }

private:
    const QString _configFileName;

    QString _errorString;

    //[DATABASE]
    Common::DBConnectionInfo _db_ConnectionInfo;

    //[LEVELGAUDE]
    QString _lg_Host;
    quint16 _lg_Port = 0;
    quint16 _lg_TLS = 2;
    QList<uint8_t> _lg_Address;
    QString _lg_userName;
    QString _lg_password;

    //[SYSTEM]
    int _sys_Interval = 60 * 1000;
    bool _sys_DebugMode = false;

    //[SERVER]
    QString _srv_Host;
    quint16 _srv_Port = 0;
    QString _srv_UserName;
    QString _srv_Password;
    int _srv_MaxRecord = 100;
//    int _srv_LastTankConfigID = 0;
//   int _srv_LastTankMeasumentID = 0;
};

} //namespace LevelGaugeStatus

#endif // TCONFIG_H
