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
#include "podlink.h"

//CRC32 helper function for PoolmateLive protocol
// Non-optimised crc32 calc.
// Doesn't need to be fast so leave readable.
uint32_t crc32a(unsigned char *message, int len)
{
    int i, j;
    uint32_t crc;
    char byte;

    for (crc=~0,i=0; i<len; ++i)
    {
        byte = message[i];
        for (j = 0; j <= 7; j++)
        {
            if ((crc>>24 ^ byte) & 0x80)
                crc = (crc << 1) ^ 0x04C11DB7;
            else
                crc = crc << 1;
            byte = byte << 1;
        }
    }
    return ~crc;
}

namespace
{
    void dataToWorkouts( unsigned char *buf, std::vector<ExerciseSet>& data )
    {
        // some sort of checksum exists in 0x1000,1001,1002,1003
        // need to work out how to validate it.
        // just do based on length of transfer for now.

        /* seems to be missing from poda transfer so skip.
           if (usb->len() != 4104)
           return;
           int version = buf[0x1004];
           int user = buf[0x1005];
        */
        int user = 1;

//TODO tidy buffer looping logic.
//at the moment will only work if the header doesn't wrap.
        for (int i=0; i<= 4096;)
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

};

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
    //wait();
    if (state != STARTUP)
        poolmate_cleanup();
}

void PodOrig::run()
{
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
                    emit info(QString("Transferring"));

                emit progress( 4104*100/(1+count) );
            }
            count = poolmate_len();
            emit progress( 4104*100/(1+count) );

            if (count < 4104)
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

// Type A pod interface

PodA::PodA() : serialPort(NULL)
{
}

bool PodA::init()
{
    serialPortName=this->find();

    if(serialPortName==QString(""))
    {
        state = ERROR;
        emit error("Unable to locate TypeA pod.");
        return false;
    }

    serialPort= new QSerialPort();
    serialPort->setPortName(serialPortName);

    //m_standardOutput= &out;
    if (!serialPort->open(QIODevice::ReadWrite)) {
        emit error("Unable to open serial device, check permissions.");
        state = ERROR;
        return false;
    }

    int serialPortBaudRate = QSerialPort::Baud115200;
    if (!serialPort->setBaudRate(serialPortBaudRate)  ||
        !serialPort->setDataBits(QSerialPort::Data8)  ||
        !serialPort->setParity(QSerialPort::NoParity) ||
        !serialPort->setStopBits(QSerialPort::OneStop)||
        !serialPort->setFlowControl(QSerialPort::SoftwareControl))
    {
        emit error("Unable to set serial port parameters.");
        state = ERROR;
        return false;
    }

    readData.clear();
    connect(serialPort, SIGNAL(readyRead()), SLOT(handleReadyRead()));
    connect(serialPort, SIGNAL(error(QSerialPort::SerialPortError)), SLOT(handleError(QSerialPort::SerialPortError)));

    return true;
}

QString PodA::find()
{
    QList<QSerialPortInfo> serialPortInfoList = QSerialPortInfo::availablePorts();
    foreach (const QSerialPortInfo &serialPortInfo, serialPortInfoList)
    {
        if(serialPortInfo.hasProductIdentifier()
           && serialPortInfo.productIdentifier()==0x8b30
           && serialPortInfo.hasVendorIdentifier()
           && serialPortInfo.vendorIdentifier()==0x403)
            return serialPortInfo.systemLocation();
    }
    return QString("");
}

PodA::~PodA()
{
    if(serialPort != NULL && serialPort->isOpen())
        serialPort->close();

    delete serialPort;
}

void PodA::run()
{
    int count;

    while (state != ERROR &&
           state != DONE )
    {
        switch (state)
        {
        case STARTUP:
            serialPort->flush();
            serialPort->setDataTerminalReady(false);
            serialPort->setRequestToSend(false);
            state=INITIALISED;
            break;

        case INITIALISED:
            state = READY;
            emit info(QString("Ready to transfer. Place PoolMate on pod"));
            break;

        case READY:
            while (state == READY || state == TRANSFER)
            {
                count=0;
                count = len();
                if (count)
                    emit info(QString("Transferring (%1)").arg(count));

                emit progress( 4096*100/(1+count) );

                yieldCurrentThread();
            }

            count = len();
            emit progress( 4096*100/(1+count) );

            if (count < 4096)
            {
                emit info(QString("Problem during transfer. Incomplete data."));
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
    }

}

void PodA::handleReadyRead()
{
    readData.append(serialPort->readAll());
    int len = readData.length();

    if(len>0)
    {
        this->state=TRANSFER;
    }

    if ( readData.length() > 4096)
    {
        this->state = ERROR;
        return;
    }

    if ( readData.length() == 4096)
    {
        INFO("\nDone\n");
        this->state = DONE;
        return;
    }
}

void PodA::getData(std::vector<ExerciseSet>& data)
{
    if (readData.isEmpty())
        return;

    unsigned char *buf = (unsigned char*) readData.data();
    dataToWorkouts(buf, data);
}

int PodA::len()
{
    return readData.length();
}


void PodA::handleError(QSerialPort::SerialPortError serialPortError)
{
    if (serialPortError == QSerialPort::ReadError)
    {
        state=ERROR;
    }
}

