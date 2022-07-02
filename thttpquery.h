#ifndef THTTPQUERY_H
#define THTTPQUERY_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QString>

namespace LevelGauge {

class THTTPQuery final : public QObject
{
    Q_OBJECT

public:
    static THTTPQuery* HTTPQuery(const QString& url = "", QObject* parent = nullptr) {
        static THTTPQuery* _HTTPQuery = nullptr;

        if (_HTTPQuery == nullptr) {
            _HTTPQuery = new THTTPQuery(url, parent);
        }
        return _HTTPQuery;
    }

public:
    explicit THTTPQuery(const QString& url, QObject* parent = nullptr);
    ~THTTPQuery();

public slots:
    void send(const QByteArray& data); //запускает отправку запроса

signals:
    void getAnswer(const QByteArray& Answer);
    void errorOccurred(const QString& Msg);

private slots:
    void replyFinished(QNetworkReply* resp); //конец приема ответа

private:
    QNetworkAccessManager* _manager = nullptr; //менеджер обработки соединений
    const QString _url; //адресс на который отправляется запрос
};

} //namespace LevelGauge

#endif // THTTPQUERY_H
