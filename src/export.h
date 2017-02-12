#ifndef EXPORT_H
#define EXPORT_H

#include <QDialog>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QNetworkCookieJar>
#include <QUrl>

class DataStore;
struct Workout;

namespace Ui {
class Export;
}

class Export : public QDialog
{
    Q_OBJECT

public:
    explicit Export(QWidget *parent = 0);
    void setDataStore( DataStore *_ds);
    ~Export();

    bool uploadToStrava(const QString& filename);
    bool uploadToGarmin(const QString& filename);

private slots:
    void on_fitButton_clicked();
    void on_FITChk_clicked(bool checked);
    void on_shareButton_clicked();
    void on_closeButton_clicked();

private:
    bool initializeGarminCookies();

    bool changed;
    DataStore *ds;
    Ui::Export *ui;

    QString stravaToken;

    QNetworkAccessManager manager;

    bool garminCookies;
    QString garminUser;
    QString garminPass;

    QString flowExecutionKey;
    QString ticket;

};

#endif // EXPORT_H
