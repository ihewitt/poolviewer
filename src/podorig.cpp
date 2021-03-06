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

//Move poolmate.c details into here

extern "C"
{
#include "poolmate.h"
};

#include <inttypes.h>
#include "logging.h"
#include "podorig.h"

void dataToWorkouts(const unsigned char *buf, std::vector<ExerciseSet>& data )
{
    // some sort of checksum exists in 0x1000,1001,1002,1003
//    int version = buf[0x1004];
    int user = buf[0x1005];

#ifdef VERBOSE_DEBUG
    DEBUG("Transfer dump\n");
    for (int i=0; i< (4096+8); i++)
    {
        DEBUG("%02x ", buf[i]);
        if ((i&0x7) == 0x7 && buf[i-1] == 0)
        {
            DEBUG("\n");
        }
    }
#endif

//TODO tidy buffer looping logic.
//at the moment will only work if the header doesn't wrap.
    for (int i=0; i< 4096;)
    {
        if (buf[i] == 0 && buf[i+1] == 0)
        {
            int year, month, day;
            int hour, min, sec;
            int cal, pool;
            QString type;
            QString units;

            year  = buf[i+2] + 2000;
            month = buf[i+4];
            day   = buf[i+6];
            QDate date(year, month, day);

            hour  = buf[i+3] & 0x7f;
            min   = buf[i+5] & 0x7f;
            QTime time(hour, min);

            hour  = buf[i+7] & 0x7f;
            min   = buf[i+9] & 0x7f;
            sec   = buf[i+11] & 0x7f;
            QTime t_dur(hour, min, sec);

            int sets   = buf[i+10];

            cal = ((buf[i+13] & 0x7f) << 8) | (buf[i+12]);
            pool = buf[i + 8];
            if (buf[i + 14] == 0)
            {
                units = "m";
            }
            else
            {
                units = "yd";
            }

            // step over header
            i += 16;

            if (buf[i+7] == 0x80)
            {
                type = "Swim";

                int total_len=0;
                for (int j = 0; j<sets; j++)
                {
                    int c = (i+j*8)%4096;

                    int len = ((buf[c + 2] & 0x7f) << 8) |
                        buf[c + 0];

                    total_len += len;
                }

                for (int j = 0; j<sets; j++)
                {
                    int c = (i+j*8)%4096;
                    for (int ct = 0; ct<8; ++ct)
                    {
                        DEBUG("%02x ", buf[c+ct]);
                    }
                    DEBUG(" - ");

                    hour = buf[c + 1] & 0x7f;
                    min = buf[c + 3] & 0x7f;
                    sec = buf[c + 5] & 0x7f;
                    QTime dur(hour, min, sec);

                    int secs = ((hour*60)+min)*60+sec;

                    int len = ((buf[c + 2] & 0x7f) << 8) |
                        buf[c + 0];

                    int str = ((buf[c + 6] & 0x7f) << 8) |
                        buf[c + 4];

                    ExerciseSet set;
                    set.sync = 0;
                    set.user = user;
                    set.date = date;
                    set.time = time;
                    set.type = type;
                    set.totalduration = t_dur;
                    set.set = j+1;
                    set.duration = dur;

                    set.cal = cal;
                    set.unit = units;

                    set.pool = pool;

                    set.lengths = total_len;
                    set.totaldistance = total_len * pool;

                    set.dist = len * pool;
                    set.lens = len;

                    set.strk = str;

                    if (len)
                    {
                        set.speed = 100*secs / set.dist;
                        set.effic = ((25 * secs / len) + (25 * str)) / pool;
                        if (secs)
                            set.rate = (60 * str * len) / secs;
                        else
                            set.rate = 0;
                    }
                    else
                    {
                        set.speed=0;
                        set.effic=0;
                        set.rate=0;
                    }

                    DEBUG(" %d %d %d, %d %d %d \n", hour, min, sec, pool, len, secs);

                    data.push_back(set);
                }
                i += sets * 8;
            }
            else if (buf[i+7] == 0x82)
            {
                type = "Chrono";

                for (int j = 0; j < sets; j++)
                {
                    int c = (i+j*8)%4096;

                    hour = buf[c + 1] & 0x7f;
                    min = buf[c + 3] & 0x7f;
                    sec = buf[c + 5] & 0x7f;
                    QTime dur(hour, min, sec);

                    ExerciseSet set;
                    set.sync = 0;
                    set.user = user;
                    set.date = date;
                    set.time = time;
                    set.type = type;
                    set.totalduration = t_dur;
                    set.set = j+1;
                    set.duration = dur;

                    data.push_back(set);
                }
                i += sets * 8;
            }
        }
        else
        {
            i += 8;
        }
    }
}

// TODO reorganise properly and get rid of our horrible state machine now we know
// what we need to do.

PodOrig::PodOrig()
{}

//noop
bool PodOrig::init()
{
    return true;
}

PodOrig::~PodOrig()
{
}

void PodOrig::stop()
{
    if (state == READY)
    {
        state = ERROR;
        while (state == ERROR)
        {
            sleep(1);
        }
    }
    emit info("Stopped.");
}

void PodOrig::run()
{
    if (!init())
        return;

    int count;
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
            while (state == READY && poolmate_run() > 0)
            {
                count=0;
                count = poolmate_len();
                if (count)
                {
                    emit info(QString("Transferring"));
                    emit progress( count*100/(4096+8) );
                }
            }
            count = poolmate_len();
            emit progress( count*100/(4096+8));

            if (count < (4096+8))
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

        case ERROR:
        case TRANSFER:
        case DONE:
        default:
            break;
        }

        yieldCurrentThread();
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

void PodOrig::getData( std::vector<ExerciseSet>& data )
{
    unsigned char *buf = poolmate_data();

    if (!buf)
        return;

    dataToWorkouts(buf, data);
}
