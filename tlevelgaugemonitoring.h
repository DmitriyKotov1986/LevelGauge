#ifndef TLEVELGAUGEMONITORING_H
#define TLEVELGAUGEMONITORING_H

//Qt
#include <QObject>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QTimer>
#include <QTime>
#include <QStringList>
//My
#include "tconsole.h"
#include "tlevelgauge.h"
#include "thttpquery.h"
#include "tconfig.h"

namespace LevelGauge {

typedef enum {
    CODE_OK = 0,
    CODE_ERROR = 1,
    CODE_INFORMATION = 2
} MSG_CODE;

class TLevelGaugeMonitoring final : public QObject
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

    void sendLogMsg(const uint16_t category, const QString& msg); //Сохранения логов
    void saveLogToFile(const QString& msg); //сохраняет сообщение в файл исли произошел сбой записи лога в БД
    void saveTanksMasumentToDB(const TLevelGauge::TTanksMeasuments& tanksMeasument); //Сохранение Измерений в БД
    void saveTanksConfigToDB(const TLevelGauge::TTanksConfigs& tanksConfig); //Сохранение конфирурации резервуаров в БД

    void DBQueryExecute(const QString& queryText); //ывполняет запрос к DB
    void DBCommit(); //выполняет commit db
    void errorDBQuery(const QSqlQuery& query); //обрабатывает ошибку выполнения запроса к DB

private slots:
    //консоль
    void getCommand(const QString& cmd);  //получена команда из консоли
    //уровнемер
    void getTanksMeasument(const TLevelGauge::TTanksMeasuments& tanksMeasument);
    void getTanksConfig(const TLevelGauge::TTanksConfigs& tankConfig);
    void errorOccurredLG(const QString& msg);
    //HTTP
    void sendToHTTPServer();
    void getAnswerHTTP(const QByteArray &answer); //получен ответ от сервеера
    void errorOccurredHTTP(const QString& msg); //ошибка передачи данных на се
    QString timeFromStart() const;

private:
    TConsole* _console = nullptr;  //консоль
    TLevelGauge* _levelGauge = nullptr; //Класс обработчика уровнемера
    TConfig* _cnf = nullptr; //настройки
    QSqlDatabase _db; //Промежуточная БД
    THTTPQuery* _HTTPQuery = nullptr;

    QTimer* _sendHTTPTimer = nullptr; //таймер отправки HTTP запросов
    bool _sending = false; //флаг что в текущий момент идет перылка данных.

    QStringList _sendingTanksMasumentsID;//ID записей в таблице Измерений которые сейчас отправляются
    QStringList _sendingTanksConfigsID;  //ID записей в таблице конфигурации емкостей которые сейчас отправляются
    quint64 _maxTanksMasumentsID = 0;
    quint64 _maxTanksConfigsID = 0;

    QTime _timer = QTime::currentTime(); //таймер отсчитывающий время с момента запуска (нужен для вывода отладки)
};

} //namespace LevelGauge

#endif // TLEVELGAUGEMONITORING_H
