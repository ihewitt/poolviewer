#include "authprivate.h"
#include "authdialog.h"

#include <QSettings>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QMessageBox>


AuthDialog::AuthDialog(QWidget * parent, Qt::WindowFlags f)
: QDialog(parent, f)
{
    setWindowTitle(tr("Authorize"));

    // Create a webpage to auth
    layout = new QVBoxLayout();
    layout->setSpacing(0);
    layout->setContentsMargins(2,0,2,2);
    setLayout(layout);

    view = new QWebEngineView();

    view->setContentsMargins(0,0,0,0);
    view->page()->view()->setContentsMargins(0,0,0,0);
    view->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    view->setAcceptDrops(false);
    layout->addWidget(view);

    QString urlstr = "";

    urlstr = QString("https://www.strava.com/oauth/authorize?");
    urlstr.append("client_id=").append(STRAVA_CLIENT_ID).append("&");
    urlstr.append("scope=view_private,write&");
    urlstr.append("redirect_uri=http://localhost/&");
    urlstr.append("response_type=code&");
    urlstr.append("approval_prompt=force");

    QUrl url = QUrl(urlstr);
    view->setUrl(url);
    // connects
    connect(view, SIGNAL(urlChanged(const QUrl&)), this,
            SLOT(urlChanged(const QUrl&)));
    connect(view, SIGNAL(loadFinished(bool)), this,
            SLOT(loadFinished(bool)));

}

//Callback
void
AuthDialog::urlChanged(const QUrl &url)
{
    if (url.toString().contains("localhost") &&
        url.toString().contains("code="))
    {
        QUrlQuery quq(url);
        QString code =quq.queryItemValue("code");

        QString urlstr = QString("https://www.strava.com/oauth/token");

        QUrlQuery params;
        params.addQueryItem("client_id", STRAVA_CLIENT_ID);
        params.addQueryItem("client_secret", STRAVA_SECRET);
        params.addQueryItem("code", code);

        QUrl url = QUrl(urlstr);
        QNetworkRequest request = QNetworkRequest(url);
        request.setHeader(QNetworkRequest::ContentTypeHeader,"application/x-www-form-urlencoded");

        // now get the final token - but ignore errors
        manager = new QNetworkAccessManager(this);
        connect(manager, SIGNAL(sslErrors(QNetworkReply*, const QList<QSslError> & )), this, SLOT(onSslErrors(QNetworkReply*, const QList<QSslError> & )));
        connect(manager, SIGNAL(finished(QNetworkReply*)), this, SLOT(networkRequestFinished(QNetworkReply*)));
        manager->post(request, params.toString(QUrl::FullyEncoded).toUtf8());
    }
}

// just ignore handshake errors
void
AuthDialog::onSslErrors(QNetworkReply *reply, const QList<QSslError>&)
{
    reply->ignoreSslErrors();
}

void AuthDialog::networkRequestFinished(QNetworkReply *reply) {
    // we can handle SSL handshake errors, if we got here then some kind of protocol was agreed
    if (reply->error() == QNetworkReply::NoError || reply->error() == QNetworkReply::SslHandshakeFailedError) {
        QByteArray payload = reply->readAll(); // JSON
        QString refresh_token;
        QString access_token;

        QJsonParseError parseError;
        QJsonDocument document = QJsonDocument::fromJson(payload, &parseError);
        if (parseError.error == QJsonParseError::NoError) {
            refresh_token = document.object()["refresh_token"].toString();
            access_token = document.object()["access_token"].toString();
        }

        QSettings settings("Swim","Poolmate");
        settings.setValue("stravaToken", access_token);

        QString info = QString(tr("Strava authorization was successful."));
        QMessageBox information(QMessageBox::Information, tr("Information"), info);
        information.exec();

    } else {

        QString error = QString(tr("Error retrieving access token, %1 (%2)")).arg(reply->errorString()).arg(reply->error());
        QMessageBox oautherr(QMessageBox::Critical, tr("SSL Token Refresh Error"), error);
        oautherr.setDetailedText(error);
        oautherr.exec();
    }
    // job done, dialog can be closed
    accept();

}

void
AuthDialog::loadFinished(bool ok) {
}
