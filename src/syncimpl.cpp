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

#include "syncimpl.h"
#include "podorig.h"
#include "poda.h"
#include "podlive.h"
#include "logging.h"

// TODO tidy up usb class communication.
// tidy messages
// remove logging from poolmate.c

SyncImpl::SyncImpl( QWidget * parent, Qt::WindowFlags f)
    : QDialog(parent, f), pod(0)
{
    setupUi(this);

    progressBar->setRange(0,100);
    progressBar->setValue(0);
    start();
}

SyncImpl::~SyncImpl()
{
    if (pod)
    {
        pod->stop();
        delete pod;
    }
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
        pod = new PodOrig();
        break;
    case 1:
        pod = new PodA();
        break;
    case 2:
        pod = new PodLive();
        break;
    }

    void info(QString msg);
    void error(QString msg);
    void progress(int progress);

    connect(pod, SIGNAL(info(QString)), SLOT(podMsg(QString)));
    connect(pod, SIGNAL(error(QString)), SLOT(podMsg(QString)));
    connect(pod, SIGNAL(progress(int)), SLOT(podProgress(int)));

    pod->start();
}

