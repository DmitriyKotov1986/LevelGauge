#include "fafnir.h"

//Qt
#include <QCoreApplication>
#include <QSslConfiguration>
#include <QXmlStreamReader>

//My
#include "Common/common.h"
#include "tconfig.h"

using namespace Common;

using namespace LevelGauge;

Fafnir::Fafnir(QObject *parent)
    : TLevelGauge{parent}
    , _cnf(TConfig::config())
{
}

Fafnir::~Fafnir()
{
    Q_CHECK_PTR(_watchDoc);
    Q_CHECK_PTR(_manager);
    Q_CHECK_PTR(_getDataTimer);

    if (_manager != nullptr)
    {
        delete _manager;
    }

    if (_watchDoc != nullptr)
    {
        delete _watchDoc;
    }

    if (_getDataTimer != nullptr)
    {
        delete _getDataTimer;
    }
}

void Fafnir::start()
{
    Q_CHECK_PTR(_cnf);

    _manager = new QNetworkAccessManager(this);
    _manager->setTransferTimeout(30000);
    QObject::connect(_manager, SIGNAL(finished(QNetworkReply *)),
                     SLOT(replyFinished(QNetworkReply *))); //событие конца обмена данными
    QObject::connect(_manager, SIGNAL(authenticationRequired(QNetworkReply*, QAuthenticator*)),
                     SLOT(authenticationRequired(QNetworkReply*, QAuthenticator*)));
    QObject::connect(_manager, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError>&)),
                     SLOT(sslErrors(QNetworkReply*, const QList<QSslError>&)));

    _getDataTimer = new QTimer();
    QObject::connect(_getDataTimer, SIGNAL(timeout()), SLOT(getData()));
    _getDataTimer->start(_cnf->sys_Interval());

    //WatchDoc
    _watchDoc = new QTimer();
    QObject::connect(_watchDoc, SIGNAL(timeout()), SLOT(watchDocTimeout()));
    _watchDoc->setSingleShot(true);

    getData();
}

void Fafnir::getData()
{
    Q_CHECK_PTR(_manager);
    Q_CHECK_PTR(_cnf);

    //создаем и отправляем запрос
    QString url = QString("https://%1:%2/cgi-bin/index.cgi?cmd=loadXmlFile&dev_type=1&data_type=snapshot&units=Lk&lang=en_EN")
                    .arg(_cnf->lg_Host())
                    .arg(_cnf->lg_Port());

    QNetworkRequest request(url);
    request.setSslConfiguration(QSslConfiguration::defaultConfiguration());

    request.setAttribute(QNetworkRequest::HttpPipeliningAllowedAttribute, false);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "keep-alive");
    request.setHeader(QNetworkRequest::UserAgentHeader, QCoreApplication::applicationName());
    request.setTransferTimeout(30000);

    QNetworkReply* resp = _manager->get(request);

    writeDebugLogFile("Get levalgauge request", "");

    if (resp != nullptr)
    {
        QTimer* watchDocTimer = new QTimer();
        watchDocTimer->setSingleShot(true);
        QObject::connect(watchDocTimer, SIGNAL(timeout()), SLOT(watchDocTimeout()));
        _watchDocs.insert(resp, watchDocTimer);
        watchDocTimer->start(60000);
    }
    else
    {
        emit errorOccurred("Send levalgauge request fail");

        return;
    }
}

void Fafnir::replyFinished(QNetworkReply *resp)
{
    Q_CHECK_PTR(resp);
    Q_ASSERT(_watchDocs.contains(resp));

    if (resp->error() == QNetworkReply::NoError)
    {
        QByteArray answer = resp->readAll();

        writeDebugLogFile("ANSWER LEVELGAUGE>", answer);

        parseAnswer(answer);
    }
    else
    {
        emit errorOccurred("Request levalgauge fail. Code: " + QString::number(resp->error()) + " Msg: " + resp->errorString());
    }
    _watchDocs[resp]->stop();
    _watchDocs[resp]->deleteLater();
    _watchDocs.remove(resp);

    resp->deleteLater();
    resp = nullptr;
}

void Fafnir::watchDocTimeout()
{
    for (auto const& resp : _watchDocs.keys())
    {
        resp->abort();
    }
}

void Fafnir::authenticationRequired(QNetworkReply *reply, QAuthenticator *authenticator)
{
    Q_CHECK_PTR(authenticator);
    Q_CHECK_PTR(_cnf);
    Q_CHECK_PTR(reply);

    authenticator->setUser(_cnf->lg_userName());
    authenticator->setPassword(_cnf->lg_password());
    authenticator->setOption("realm", authenticator->realm());
}

void Fafnir::sslErrors(QNetworkReply *reply, const QList<QSslError> &errors)
{
    Q_CHECK_PTR(reply);

    reply->ignoreSslErrors();
}

void Fafnir::parseAnswer(const QByteArray &answer)
{
    QXmlStreamReader XMLReader(answer);

    while ((!XMLReader.atEnd()) && (!XMLReader.hasError()))
    {
        QXmlStreamReader::TokenType Token = XMLReader.readNext();
        if (Token == QXmlStreamReader::StartDocument)
        {
            continue;
        }
        else if (Token == QXmlStreamReader::EndDocument)
        {
            break;
        }
        else if (Token == QXmlStreamReader::StartElement)
        {
            if (XMLReader.name().toString()  == "root")
            {
                while ((XMLReader.readNext() != QXmlStreamReader::EndElement) && !XMLReader.atEnd() && !XMLReader.hasError())
                {
                    if (XMLReader.name().toString().isEmpty())
                    {
                        continue;
                    }
                    else if (XMLReader.name().toString()  == "Infos")
                    {
                        XMLReader.skipCurrentElement();
                    }
                    else if (XMLReader.name().toString()  == "Inventories")
                    {
                        while ((XMLReader.readNext() != QXmlStreamReader::EndElement) && !XMLReader.atEnd() && !XMLReader.hasError())
                        {
                            if (XMLReader.name().toString().isEmpty())
                            {
                                continue;
                            }
                            else if (XMLReader.name().toString()  == "Tab_Head")
                            {
                                XMLReader.skipCurrentElement();
                            }
                            else if (XMLReader.name().toString()  == "Tab_Labels")
                            {
                                XMLReader.skipCurrentElement();
                            }
                            else if (XMLReader.name().toString()  == "Inventory")
                            {
                                TLevelGauge::TTankMeasument tmp;
                                uint8_t number = 0;
                                while ((XMLReader.readNext() != QXmlStreamReader::EndElement) && !XMLReader.atEnd() && !XMLReader.hasError())
                                {
                                    if (XMLReader.name().toString().isEmpty())
                                    {
                                        continue;
                                    }
                                    else if (XMLReader.name().toString()  == "Prod_Name")
                                    {
                                        XMLReader.skipCurrentElement();
                                    }
                                    else if (XMLReader.name().toString()  == "Probe_Status")
                                    {
                                        XMLReader.skipCurrentElement();
                                    }
                                    else if (XMLReader.name().toString()  == "Alarms_Active")
                                    {
                                        XMLReader.skipCurrentElement();
                                    }
                                    else if (XMLReader.name().toString()  == "Volume_Tc")
                                    {
                                        XMLReader.skipCurrentElement();
                                    }
                                    else if (XMLReader.name().toString()  == "Ullage")
                                    {
                                        XMLReader.skipCurrentElement();
                                    }
                                    else if (XMLReader.name().toString()  == "Water_Vol")
                                    {
                                        XMLReader.skipCurrentElement();
                                    }
                                    else if (XMLReader.name().toString()  == "Prod_DensityTc")
                                    {
                                        XMLReader.skipCurrentElement();
                                    }
                                    else if (XMLReader.name().toString()  == "Density_Temp")
                                    {
                                        XMLReader.skipCurrentElement();
                                    }
                                    else if (XMLReader.name().toString()  == "Smp_Density")
                                    {
                                        XMLReader.skipCurrentElement();
                                    }
                                    else if (XMLReader.name().toString()  == "Smp_DensityTc")
                                    {
                                        XMLReader.skipCurrentElement();
                                    }
                                    else if (XMLReader.name().toString()  == "Dlv_Start")
                                    {
                                        XMLReader.skipCurrentElement();
                                    }
                                    //number
                                    else if (XMLReader.name().toString()  == "Tank_Nr")
                                    {
                                        bool ok = true;
                                        number = XMLReader.readElementText().toULongLong(&ok);
                                        if (!ok)
                                        {
                                            emit errorOccurred(QString("-->Incorrect value tag (root/Inventories/Inventory/%1). Value: %2. Value must be numeric.").arg(XMLReader.name().toString()).arg(XMLReader.readElementText()));
                                        }
                                    }
                                    //volume
                                    else if (XMLReader.name().toString()  == "Volume")
                                    {
                                        bool ok = true;
                                        tmp.volume = static_cast<decltype(tmp.volume)>(XMLReader.readElementText().toDouble(&ok));
                                        if (!ok)
                                        {
                                            emit errorOccurred(QString("-->Incorrect value tag (root/Inventories/Inventory/%1). Value: %2. Value must be numeric.").arg(XMLReader.name().toString()).arg(XMLReader.readElementText()));
                                        }
                                    }
                                    //mass
                                    else if (XMLReader.name().toString()  == "Mass")
                                    {
                                        bool ok = true;
                                        tmp.mass = static_cast<decltype(tmp.mass)>(XMLReader.readElementText().toDouble(&ok));
                                        if (!ok)
                                        {
                                            emit errorOccurred(QString("-->Incorrect value tag (root/Inventories/Inventory/%1). Value: %2. Value must be numeric.").arg(XMLReader.name().toString()).arg(XMLReader.readElementText()));
                                        }
                                    }
                                    //height
                                    else if (XMLReader.name().toString()  == "Prod_Level")
                                    {
                                        bool ok = true;
                                        tmp.height = static_cast<decltype(tmp.height)>(XMLReader.readElementText().toDouble(&ok));
                                        if (!ok)
                                        {
                                            emit errorOccurred(QString("-->Incorrect value tag (root/Inventories/Inventory/%1). Value: %2. Value must be numeric.").arg(XMLReader.name().toString()).arg(XMLReader.readElementText()));
                                        }
                                    }
                                    //Temp
                                    else if (XMLReader.name().toString()  == "Temp")
                                    {
                                        bool ok = true;
                                        tmp.temp = static_cast<decltype(tmp.temp)>(XMLReader.readElementText().toDouble(&ok));
                                        if (!ok)
                                        {
                                            emit errorOccurred(QString("-->Incorrect value tag (root/Inventories/Inventory/%1). Value: %2. Value must be numeric.").arg(XMLReader.name().toString()).arg(XMLReader.readElementText()));
                                        }
                                    }
                                    //water
                                    else if (XMLReader.name().toString()  == "Water_Lvl")
                                    {
                                        bool ok = true;
                                        tmp.water = static_cast<decltype(tmp.water)>(XMLReader.readElementText().toDouble(&ok));
                                        if (!ok)
                                        {
                                            emit errorOccurred(QString("-->Incorrect value tag (root/Inventories/Inventory/%1). Value: %2. Value must be numeric.").arg(XMLReader.name().toString()).arg(XMLReader.readElementText()));
                                        }
                                    }
                                    //water
                                    else if (XMLReader.name().toString()  == "Prod_Density")
                                    {
                                        bool ok = true;
                                        tmp.density = static_cast<decltype(tmp.density)>(XMLReader.readElementText().toDouble(&ok));
                                        if (!ok)
                                        {
                                            emit errorOccurred(QString("-->Incorrect value tag (root/Inventories/Inventory/%1). Value: %2. Value must be numeric.").arg(XMLReader.name().toString()).arg(XMLReader.readElementText()));
                                        }
                                    }
                                    else
                                    {
                                        emit errorOccurred(QString("Undefine tag in XML (root/Inventories/Inventory/%1)").arg(XMLReader.name().toString()));

                                        XMLReader.readElementText();
                                    }
                                }

                                tmp.dateTime = QDateTime::currentDateTime();

                                //проверям полученные значения
                                if (!checkMeasument(number, tmp))
                                {
                                    continue;
                                }

                                _tanksMeasuments.emplace(number, tmp);
                            }
                            else
                            {
                                emit errorOccurred(QString("Undefine tag in XML (root/Inventories/%1)").arg(XMLReader.name().toString()));

                                XMLReader.readElementText();
                            }
                       }
                    }
                    else
                    {
                        emit errorOccurred(QString("Undefine tag in XML (root/%1)").arg(XMLReader.name().toString()));

                        XMLReader.readElementText();
                    }
                }
            }
            else
            {
                emit errorOccurred(QString("Undefine tag in XML (%1)").arg(XMLReader.name().toString()));

                XMLReader.readElementText();
            }
        }
    }

    if (XMLReader.hasError()) { //неудалось распарсить пришедшую XML
        emit errorOccurred(QString("Incorrect answer from server. Parser msg: %1 Answer from server: %2").arg(XMLReader.errorString()).arg(answer));
    }

    if (!_tanksMeasuments.isEmpty())
    {
        emit getTanksMeasument(_tanksMeasuments);
        _tanksMeasuments.clear();
    }
}
