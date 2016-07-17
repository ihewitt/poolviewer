/*
 * This file is part of PoolViewer
 * Copyright (c) 2011-2015 Ivor Hewitt
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

#include <stdio.h>

#include <QTimer>
#include <QPushButton>

#include "syncimpl.h"
#include "podorig.h"
#include "poda.h"
#include "podlive.h"
#include "logging.h"

// TODO tidy up usb class communication.
// tidy messages
// remove logging from poolmate.c

SyncImpl::SyncImpl( QWidget * parent)
    : QDialog(parent, Qt::CustomizeWindowHint | Qt::WindowTitleHint), pod(0)
{
    // make sure Qt::WindowCloseButtonHint is NOT specified
    // as it would closr this dialog before the thread is finisehd

    setupUi(this);

    progressBar->setRange(0,100);
    progressBar->setValue(0);
    start();
}

void SyncImpl::podMsg(QString msg)
{
    label->setText(msg);
}

void SyncImpl::podProgress(int progress)
{
    progressBar->setValue(progress);
    update();
}

void SyncImpl::podFinished()
{
    // we know the thread has sent a finished signal
    // is it worth waiting as well? to ensure it is really done
    pod->wait();

    // ok, the thread is really done now, allow the dialog to be closed
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void SyncImpl::getData(std::vector<ExerciseSet>& data)
{
    pod->getData( data );
}

//
//
void SyncImpl::start()
{
    QSettings settings("Swim","Poolmate");
    QString path = settings.value("dataFile").toString();

    //
    // Instantiate appropriate serial interface backend.
    //
    int podType = settings.value("podType").toInt(); //use int in case we need to add another
    switch (podType)
    {
    case 0:
        pod.reset(new PodOrig());
        break;
    case 1:
        pod.reset(new PodA());
        break;
    case 2:
        pod.reset(new PodLive());
        break;
    }

    connect(pod.data(), SIGNAL(info(QString)), SLOT(podMsg(QString)));
    connect(pod.data(), SIGNAL(error(QString)), SLOT(podMsg(QString)));
    connect(pod.data(), SIGNAL(progress(int)), SLOT(podProgress(int)));
    connect(pod.data(), SIGNAL(finished()), SLOT(podFinished()));

    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
    buttonBox->button(QDialogButtonBox::Abort)->setEnabled(true);

    pod->start();
}


void SyncImpl::on_buttonBox_rejected()
{
    // this is just a request
    // we will wait for a finished signal
    pod->stop();

    // no point in stopping more than once
    buttonBox->button(QDialogButtonBox::Abort)->setEnabled(false);
}
