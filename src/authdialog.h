#ifndef AUTHDIALOG_H
#define AUTHDIALOG_H

#include <QObject>
#include <QDialog>
#include <QtGui>
#include <QWidget>
#include <QStackedLayout>
#include <QUrl>
#include <QSslSocket>

#include <QUrlQuery>
#include <QWebEngineHistory>
#include <QWebEngineHistoryItem>
#include <QWebEnginePage>
#include <QWebEngineView>


class AuthDialog : public QDialog
{
Q_OBJECT

public:
    AuthDialog(QWidget * parent = 0, Qt::WindowFlags f = Qt::WindowFlags() );

private slots:
    void urlChanged(const QUrl& url);
    void loadFinished(bool ok);
    void networkRequestFinished(QNetworkReply *reply);
    void onSslErrors(QNetworkReply *reply, const QList<QSslError>&);

private:
    QNetworkAccessManager* manager;
    QVBoxLayout *layout;
    QWebEngineView *view;
};

#endif // AUTHDIALOG_H
