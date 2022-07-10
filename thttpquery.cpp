#include "thttpquery.h"

//Qt
#include <QCoreApplication>

//My
#include "common.h"

using namespace LevelGauge;

THTTPQuery::THTTPQuery(const QString& url, QObject* parent)
    : QObject(parent)
    , _manager(new QNetworkAccessManager(this))
    , _url(url)
{
    Q_ASSERT(_manager);

    _manager->setTransferTimeout(30000);
    QObject::connect(_manager, SIGNAL(finished(QNetworkReply *)),
                     SLOT(replyFinished(QNetworkReply *))); //событие конца обмена данными
}

THTTPQuery::~THTTPQuery()
{
    Q_ASSERT(_manager);

    _manager->disconnect();
    _manager->deleteLater();
}

void THTTPQuery::send(const QByteArray& data)
{
    //создаем и отправляем запрос
    QNetworkRequest request(_url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/xml");
    request.setHeader(QNetworkRequest::UserAgentHeader, QCoreApplication::applicationName());
    request.setHeader(QNetworkRequest::ContentLengthHeader, QString::number(data.size()));
    request.setTransferTimeout(30000);

    QNetworkReply* resp = _manager->post(request, data);

    writeDebugLogFile("HTTP request:", QString(data));

    if (resp == nullptr) {
        emit errorOccurred("Send HTTP request fail");
        return;
    }
}

void THTTPQuery::replyFinished(QNetworkReply *resp)
{
    Q_ASSERT(resp);

    if (resp->error() == QNetworkReply::NoError) {
        QByteArray answer = resp->readAll();

        writeDebugLogFile("HTTP answer:", QString(answer));

        emit getAnswer(answer);
    }
    else {
        emit errorOccurred("HTTP request fail. Code: " + QString::number(resp->error()) + " Msg: " + resp->errorString());
    }

    resp->deleteLater();
    resp = nullptr;
}



