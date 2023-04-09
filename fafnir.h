#ifndef FAFNIR_H
#define FAFNIR_H

#include "tlevelgauge.h"

//Qt
#include <QObject>
#include <QTimer>
#include <QNetworkAccessManager>
#include <QHash>
#include <QNetworkReply>
#include <QAuthenticator>

namespace LevelGauge
{

class Fafnir : public TLevelGauge
{
    Q_OBJECT

public:
    explicit Fafnir(QObject *parent = nullptr);
    ~Fafnir();

public slots:
    void start() override;

private slots:
    void getData(); //запускает отправку запроса
    void replyFinished(QNetworkReply* resp); //конец приема ответа
    void watchDocTimeout();
    void authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator);
    void sslErrors(QNetworkReply* reply, const QList<QSslError>& errors);

private:
    void parseAnswer(const QByteArray& answer);

private:
    TConfig* _cnf = nullptr;
    QTimer* _getDataTimer = nullptr;
    QTimer* _watchDoc = nullptr;

   // TLevelGauge::TTanksConfigs _tanksConfigs; //очередь конфигураций резервуаров
    TLevelGauge::TTanksMeasuments _tanksMeasuments; //очередь результатов измерений

    const QString _url; //адресс на который отправляется запрос
    QHash<QNetworkReply*, QTimer*> _watchDocs;
    QNetworkAccessManager* _manager = nullptr; //менеджер обработки соединений

};

} //namespace LevelGauge

#endif // FAFNIR_H
