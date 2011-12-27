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

#include <stdio.h>

#include <QTimer>

#include "syncimpl.h"

extern "C"
{
#include "poolmate.h"
    extern Exercise exercises[MAX_EXERCISES];
    extern int num_exercises;
};

// TODO tidy up usb class communication.
// tidy messages
// remove logging from poolmate.c

SyncImpl::SyncImpl( QWidget * parent, Qt::WFlags f) 
    : QDialog(parent, f), usb(0)
{
    setupUi(this);

    progressBar->setRange(0,512);
    progressBar->setValue(0);

    start();
}


void SyncImpl::usbMsg(QString msg)
{
    label->setText(msg);
}

void SyncImpl::usbProgress(int progress)
{
    progressBar->setValue(progress);
}


Usb::Usb()
{
    state = STARTUP;
}

Usb::~Usb()
{
    poolmate_cleanup();
}

void Usb::run()
{
    while ( state != ERROR &&
            state != DONE )
    {
        switch (state)
        {
        case STARTUP:
            if (poolmate_init())
            {
                state = ERROR;
                emit error("Unable to initialise USB.");
            }
            else if (poolmate_find())
            {
                state = ERROR;
                emit error(QString("Unable to find PoolMate device."));
            }
            else if (poolmate_attach())
            {
                state = ERROR;
                emit error(QString("Unable to talk to device (check permissions)."));
            }
            
            if (state != ERROR)
                state = INITIALISED;
            break;
            
        case INITIALISED:
            if (poolmate_start())
            {
                state = ERROR;
                emit error(QString("Unable to download data."));
            }
            if (state != ERROR)
            {
                state = READY;
                emit info(QString("Ready to transfer. Place PoolMate on pod"));
            }
            break;
            
        case READY:
            int count=0;
            while (poolmate_run() > 0)
            {
                count = poolmate_progress();
                if (count)
                    emit info(QString("Transferring"));

                emit progress( count );
            }

            count = poolmate_progress();
            emit progress( count );

            if (count < 512)
            {
                emit info(QString("Problem during transfer."));
                state = ERROR;
            }
            else
            {
                emit info(QString("Transfer complete."));
                state = DONE;
            }
            break;
        }
    }

    if (state == ERROR)
    {
        poolmate_cleanup();
        state = STARTUP;
    }

    if (state == DONE)
    {
        poolmate_stop();
        state = INITIALISED;
    }
}

void SyncImpl::getData(std::vector<ExerciseSet>& data)
{
    // extract and flatten exercise data 
    for (int i=0; i<=num_exercises; i++)
    {
        Exercise *e = &exercises[i];

        for (int j=0;j<e->sets_t;j++)
        {
            Set *s = &exercises[i].sets[j];

            ExerciseSet set;
            set.user = e->user;
            set.date = QDate(e->year, e->month, e->day);
            set.time = QTime(e->hour, e->min);

            if (s->status == 0) //swim
            {
                set.type ="Swim";
                set.pool = e->pool;
                set.unit = "m"; //TODO find and extract
                set.totalduration = QTime(e->dur_hr, e->dur_min, e->dur_sec);
                set.cal = e->cal;
                set.lengths = e->len_t;
                set.totaldistance = e->len_t * e->pool;
                set.set = j+1;
                set.duration = QTime(s->hr, s->min, s->sec);
                set.strk = s->str;
                set.dist = s->len * e->pool;
                set.lens = s->len;
                int time_secs = (((s->hr*60)+s->min)*60+s->sec);
                set.speed = 100*time_secs/(s->len*e->pool);
                set.effic = ((25*time_secs/s->len)+(25*s->str))/e->pool;
                set.rate = (60*s->str*s->len) / time_secs;
            }
            else
            {
                set.type="Chrono";
                set.totalduration = QTime(e->dur_hr, e->dur_min, e->dur_sec);
                set.set = j+1;
                set.duration = QTime(s->hr, s->min, s->sec);
            }

            data.push_back(set);
        }
    }
}

void SyncImpl::start()
{
    usb = new Usb();
    void info(QString msg);
    void error(QString msg);
    void progress(int progress);

    connect(usb, SIGNAL(info(QString)), SLOT(usbMsg(QString)));
    connect(usb, SIGNAL(error(QString)), SLOT(usbMsg(QString)));
    connect(usb, SIGNAL(progress(int)), SLOT(usbProgress(int)));

    usb->start();
}

