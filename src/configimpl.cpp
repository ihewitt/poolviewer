/*
 * This file is part of PoolViewer
 * Copyright (c) 2011 Ivor Hewitt
 *
 * PoolViewer is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * PoolViewer is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with PoolViewer.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QSettings>

#include "authdialog.h"
#include "configimpl.h"

ConfigImpl::ConfigImpl( QWidget * parent, Qt::WindowFlags f)
    : QDialog(parent, f)
{
    setupUi(this);

    QSettings settings("Swim","Poolmate");
    QString path = settings.value("dataFile").toString();
    int pod = settings.value("podType").toInt(); //use int in case we need to add another

    if (pod == 0) // 0 - original, 1 - type A
    {
        podOriginal->setChecked(true);
        podorig->setEnabled(true);
        //orighelp->setVisible(true);
        //livehelp->setVisible(false);
    }
    else if (pod == 1)
    {
        podTypeA->setChecked(true);
        poda->setEnabled(true);

        //orighelp->setVisible(false);
        //livehelp->setVisible(true);
    }
    else
    {
        podLive->setChecked(true);
        podlive->setEnabled(true);

        //orighelp->setVisible(false);
        //livehelp->setVisible(true);
    }

    QString stravaToken = settings.value("stravaToken").toString();
    if (stravaToken.isEmpty())
        stravaAuth->setIcon(QIcon());// clear icon
    else
        stravaAuth->setIcon(QIcon(":/images/tick.png"));

    dataFile->setText(path);
    backup->setChecked(settings.value("backup").toBool());

    garminUser->setText(settings.value("garminUser").toString());
    garminPassword->setText(settings.value("garminPass").toString());

}


void ConfigImpl::on_buttonBox_accepted()
{
    QSettings settings("Swim","Poolmate");

    int podsetting;
    if (podOriginal->isChecked())
    {
        podsetting = 0;
    }
    else if (podTypeA->isChecked())
    {
        podsetting = 1;
    }
    else
    {
        podsetting = 2;
    }

    int pod = settings.value("podType").toInt(); //use int in case we need to add another
    if (podsetting != pod)
    {
        settings.setValue("podType", podsetting);
    }

    QString path = settings.value("dataFile").toString();
    if ( path.compare(dataFile->text() ) != 0)
    {
        settings.setValue("dataFile", dataFile->text());
    }

    settings.setValue("backup", backup->isChecked());

    settings.setValue("garminUser", garminUser->text());
    settings.setValue("garminPass", garminPassword->text());
}

void ConfigImpl::on_podLive_clicked(bool /*checked*/)
{
    podorig->setEnabled(false);
    poda->setEnabled(false);
    podlive->setEnabled(true);

    //orighelp->setVisible(false);
    //livehelp->setVisible(true);
}

void ConfigImpl::on_podOriginal_clicked(bool /*checked*/)
{
    podorig->setEnabled(true);
    poda->setEnabled(false);
    podlive->setEnabled(false);

    //orighelp->setVisible(true);
    //livehelp->setVisible(false);
}

void ConfigImpl::on_podTypeA_clicked(bool /*checked*/)
{
    podorig->setEnabled(false);
    poda->setEnabled(true);
    podlive->setEnabled(false);

    //orighelp->setVisible(false);
    //livehelp->setVisible(true);
}

void ConfigImpl::on_stravaAuth_clicked()
{
    AuthDialog win(this);
    /*int ret =*/ win.exec();

    QSettings settings("Swim","Poolmate");
    QString stravaToken = settings.value("stravaToken").toString();
    if (!stravaToken.isEmpty())
        stravaAuth->setIcon(QIcon(":/images/tick.png"));
}
