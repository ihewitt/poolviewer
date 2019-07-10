#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QUrl>
#include <QUrlQuery>
#include <QHttpMultiPart>
#include <QNetworkReply>
#include <QNetworkCookie>
#include <QNetworkCookieJar>

#include "export.h"
#include "ui_export.h"
#include "datastore.h"

#include <unistd.h>

Export::Export(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Export)
{
    ui->setupUi(this);

    QSettings settings("Swim","Poolmate");
    QString dirname=settings.value("fitDir").toString();

    stravaToken = settings.value("stravaToken").toString();
    garminUser = settings.value("garminUser").toString();
    garminPass =settings.value("garminPass").toString();

    ui->targetDir->setText(dirname);
    ui->FITChk->setChecked(true);

    ui->stravaChk->setEnabled(!stravaToken.isEmpty());
    ui->garminChk->setEnabled(!garminUser.isEmpty());

    garminCookies = false;
    changed = false;

    ui->progressBar->setValue(0);
    ui->progressBar->setMinimum(0);
    ui->progressBar->setMaximum(0);

    ui->allButton->setChecked(true);

    manager=0;
    eventLoop=0;
}

Export::~Export()
{
    if (manager)
        delete manager;

    if (eventLoop)
        delete eventLoop;

    delete ui;
}

void Export::setDataStore( DataStore *_ds)
{
    ds = _ds;
}

void Export::on_fitButton_clicked()
{
    QSettings settings("Swim","Poolmate");
    QString dirname=settings.value("fitDir").toString();

    //Bug, doesn't open location with default flags.
    dirname = QFileDialog::getExistingDirectory(this,
                                                tr("Select .FIT export directory."),
                                                dirname,
                                                QFileDialog::ShowDirsOnly
                                                /*QFileDialog::DontUseNativeDialog*/);
    settings.setValue("fitDir", dirname);
}

//Need fit file export
void Export::on_FITChk_clicked(bool checked)
{
    if (checked)
    {
        ui->stravaChk->setEnabled(!stravaToken.isEmpty());
        ui->garminChk->setEnabled(!garminUser.isEmpty());
    }
    else
    {
        ui->stravaChk->setEnabled(false);
        ui->garminChk->setEnabled(false);
    }
}

void Export::on_shareButton_clicked()
{
    QSettings settings("Swim","Poolmate");
    QString dirname=settings.value("fitDir").toString();
    if (dirname.isEmpty() || !QDir(dirname).exists())
    {
        QMessageBox::information(this,tr("Error"),tr("Please set a valid .FIT export directory."));
        return;
    }

    changed = false;

    ui->progressBar->setMaximum(ds->Workouts().size());

    std::vector<Workout>::const_iterator i;
    int count=1;
    for ( i = ds->Workouts().begin(); i != ds->Workouts().end(); ++i, ++count)
    {
        ui->progressBar->setValue(count);

        if (ui->todayButton->isChecked() &&
                i->date != QDate())
        {
            if (ui->flagBox->isChecked())
            {
                i->sync |= SYNC_FIT|SYNC_GARMIN|SYNC_STRAVA;
                changed=true;
            }
            continue;
        }

        if ( i->type=="SwimHR" ) //Only interested in exporting data with lengths
        {
            bool fit = false;
            QString filename;

            if (ui->FITChk->isChecked() ||
                    ui->stravaChk->isChecked() ||
                    ui->garminChk->isChecked() )  // All three require a fit file to exist
            {
                filename.clear();
                if (ds->exportWorkout(dirname, filename, *i)) //Create a  FIT file
                {
                    if (!(i->sync & SYNC_FIT))
                    {
                        i->sync |= SYNC_FIT;
                        changed=true;
                    }
                    fit=true; // we have or made a fit file
                }
            }

            if (fit && !filename.isEmpty()) // We have a fit file so upload
            {
                if (ui->stravaChk->isChecked())
                {
                    if (!(i->sync & SYNC_STRAVA))
                    {
                        if (uploadToStrava(filename))
                        {
                            i->sync |= SYNC_STRAVA;
                            changed=true;
                        }
                        else
                        {
                            QMessageBox::information(this,tr("Error"),tr("Problem uploading to Strava."));
                            break;
                        }
                    }
                }

                if (ui->garminChk->isChecked())
                {
                    if (!(i->sync & SYNC_GARMIN))
                    {
                        if (uploadToGarmin( filename ))
                        {
                            i->sync |= SYNC_GARMIN;
                            changed=true;
                        }
                        else
                        {
                            QMessageBox::information(this,tr("Error"),tr("Problem uploading to Garmin."));
                            break;
                        }

                    }
                }
            }
        }
    }

    if (changed)
    {
        ds->setChanged();
    }
}

bool Export::uploadToStrava(const QString& filename)
{
    QEventLoop eventLoop(this);// = new QEventLoop(this);

    QNetworkAccessManager networkManager(this);

    connect(&networkManager, SIGNAL(finished(QNetworkReply *)), &eventLoop, SLOT(quit()));

    QSettings settings("Swim","Poolmate");
    QString token = settings.value("stravaToken").toString();

    QUrl url = QUrl( "https://www.strava.com/api/v3/uploads" );
    QNetworkRequest request = QNetworkRequest(url);

    QString boundary = QVariant(qrand()).toString() + QVariant(qrand()).toString() + QVariant(qrand()).toString();

    QFile f(filename);
    f.open(QFile::ReadOnly);
    QByteArray file(f.readAll());

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    multiPart->setBoundary(boundary.toLatin1());

    QHttpPart accessTokenPart;
    accessTokenPart.setHeader(QNetworkRequest::ContentDispositionHeader,
                              QVariant("form-data; name=\"access_token\""));
    accessTokenPart.setBody(token.toLatin1());

    QHttpPart activityTypePart;
    activityTypePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                               QVariant("form-data; name=\"activity_type\""));
    activityTypePart.setBody("swim");

    QHttpPart dataTypePart;
    dataTypePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"data_type\""));
    dataTypePart.setBody("fit");

    //    QHttpPart privatePart;
    //    privatePart.setHeader(QNetworkRequest::ContentDispositionHeader,
    //                          QVariant("form-data; name=\"private\""));
    //    privatePart.setBody("1" );

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\"file.fit\"; type=\"application/octet-stream\""));
    filePart.setBody(file);

    multiPart->append(accessTokenPart);
    multiPart->append(activityTypePart);
    multiPart->append(dataTypePart);
    //    multiPart->append(privatePart);
    multiPart->append(filePart);
    QNetworkReply *reply = networkManager.post(request, multiPart);

    eventLoop.exec(); //Wait here until post complete

    //now check response

    //uploadSuccessful = false;
    QString response = reply->readLine();
    qDebug() << response;

    //Just do a string compare rather than anything more complex
    if (response.contains("duplicate of activity"))
    {
        return true; //already uploaded so flag as done.
    }
    else if (response.contains("activity_id"))
    {
        return true; //
    }

    return false;
}

// How is it possible to make such a mess of an http protocol
bool Export::uploadToGarmin(const QString &filename )
{
    if (!manager)
    {
        manager = new QNetworkAccessManager(this);
        manager->setCookieJar(new QNetworkCookieJar);
    }

    if (!eventLoop)
    {
        eventLoop = new QEventLoop(this);
        connect(manager, SIGNAL(finished(QNetworkReply *)), eventLoop, SLOT(quit()));
    }

    if (!garminCookies)
    {
        if (!initializeGarminCookies())
        {
            QMessageBox::information(this,tr("Error"),tr("Unable to authenticate with Garmin, check credentials in config."));
            return false;
        }
    }

    QUrl url = QUrl( "https://connect.garmin.com/modern/proxy/upload-service/upload/.fit" );

    QNetworkRequest request = QNetworkRequest(url);
    request.setRawHeader("Referer", "https://connect.garmin.com/modern/import-data");
    request.setRawHeader("NK","NT"); //Mandatory header

    QString boundary = QVariant(qrand()).toString() + QVariant(qrand()).toString() + QVariant(qrand()).toString();

    QFile f(filename);
    f.open(QFile::ReadOnly);
    QByteArray file(f.readAll());

    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    multiPart->setBoundary(boundary.toLatin1());

    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"file\"; filename=\"file.fit\""));
    filePart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("application/octet-stream"));
    filePart.setBody(file);

    multiPart->append(filePart);
    QNetworkReply *reply = manager->post(request, multiPart);
    eventLoop->exec(); //Wait here until post complete

    QString response = reply->readAll();
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

    delete reply;

    qDebug() << "response" << response <<  " " <<  statusCode;

    if (statusCode == 200 || statusCode == 201 || statusCode == 409)
    {
        //just do basic string search for results
        if (response.contains("Duplicate Activity"))
        {
            return true; //Return ok so flagged as already uploaded.
        } else if (response.contains("\"failures\":[]"))
        {
            return true;
        }
    }

    garminCookies=false; //Force relogin
    return false;
}

bool Export::initializeGarminCookies()
{
    QUrl serverUrl;
    QNetworkRequest request;
    QNetworkReply *reply;
    QString response;
    int statusCode;

    //Switched to garmin connect 'modern' calls.
    QUrl baseUrl("https://sso.garmin.com/sso/signin");

    QUrlQuery service;
    service.addQueryItem("service","https://connect.garmin.com/modern/");
    baseUrl.setQuery(service);

    QSettings settings("Swim","Poolmate");

    QUrlQuery params;
    params.addQueryItem("embed","false");
    params.addQueryItem("username",settings.value("garminUser").toString());
    params.addQueryItem("password",settings.value("garminPass").toString());
    QByteArray data = params.query(QUrl::FullyEncoded).toUtf8();

    request.setRawHeader("origin","https://sso.garmin.com");
    request.setUrl(baseUrl);

    reply =  manager->post(request, data);
    eventLoop->exec();

    qDebug() << "requestLoginGarminFinished()";

    response = reply->readAll();

    statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "statusCode " << statusCode;

    if (reply->error() != QNetworkReply::NoError)
    {
        qDebug() << "Error " << reply->error() ;
        return false;
    }
    else if (statusCode == 200)
    {
        int i = response.indexOf("?ticket=");
        if (i<=0){
            return false;
        }
        else
            i+=8;

        int j = response.indexOf("\"", i);
        ticket = response.mid(i, j-i);
        qDebug() << "ticket" << ticket;

        QUrlQuery params;
        params.addQueryItem("ticket",ticket);

        serverUrl = QUrl("https://connect.garmin.com/modern/");
        serverUrl.setQuery(params);

        int maxloop=3;
        do {
            qDebug() << "Request: " <<serverUrl;
            request.setUrl(serverUrl);
            reply = manager->get(request);
            eventLoop->exec();

            int code = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();

            if (code == 200)
            {
                garminCookies=true;
                return true;
            }

            if (code == 302)
            {
                QString loc = reply->attribute(QNetworkRequest::RedirectionTargetAttribute).toString();
                serverUrl = serverUrl.resolved(loc);
            }
        } while (maxloop--);
    }
    return false;
}

void Export::on_closeButton_clicked()
{
    close();
}
