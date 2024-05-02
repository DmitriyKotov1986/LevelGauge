#ifndef TLEVELGAUGEMONITORING_H
#define TLEVELGAUGEMONITORING_H

//Qt
#include <QObject>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QTimer>
#include <QTime>
#include <QStringList>
#include <QThread>

//My
#include "Common/tdbloger.h"

#include "tlevelgauge.h"
#include "tconfig.h"

namespace LevelGauge
{

class TLevelGaugeMonitoring final
    : public QObject
{
    Q_OBJECT

public:
    explicit TLevelGaugeMonitoring(TConfig* cnf, QObject *parent = nullptr);
    ~TLevelGaugeMonitoring();

public slots:
    void start();    //Событике запуска. Приходит сразу после запуска loop events

signals:
    void sendHTTP(const QByteArray& data); //отправка сообщения по HTTP
    void startLG(); //старт опроса уровнемера
    void finished(); //испускаем перед завершением работы

private:
    TLevelGauge* loadLG(); //возвращает класс уровнемера в зависимости от конфигурации (Фабрика)

    void saveTanksMasumentToDB(const TLevelGauge::TTanksMeasuments& tanksMeasument); //Сохранение Измерений в БД

private slots:
    //уровнемер
    void getTanksMeasument(const LevelGauge::TLevelGauge::TTanksMeasuments& tanksMeasument);
    void errorOccurredLG(const QString& msg);
    //HTTP
    void sendToHTTPServer();
    void getAnswerHTTP(const QByteArray &answer); //получен ответ от сервеера
    void errorOccurredHTTP(const QString& msg); //ошибка передачи данных на се

private:
    QThread* _levelGaugeThread = nullptr;
    TConfig* _cnf = nullptr; //настройки
    QSqlDatabase _db; //Промежуточная БД
    QThread* _HTTPQueryThread = nullptr;
    Common::TDBLoger* _loger = nullptr;

    QTimer* _sendHTTPTimer = nullptr; //таймер отправки HTTP запросов
    bool _sending = false; //флаг что в текущий момент идет перылка данных.

    QStringList _sendingTanksMasumentsID;//ID записей в таблице Измерений которые сейчас отправляются

};

} //namespace LevelGauge

#endif // TLEVELGAUGEMONITORING_H
