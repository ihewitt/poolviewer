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

#include <inttypes.h>
#include "logging.h"
#include "podlive.h"
#include <numeric>

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

///
PodLive::PodLive() : serialPort(NULL)
{
    qRegisterMetaType<QSerialPort::SerialPortError>();
}

PodLive::~PodLive()
{
    if(serialPort != NULL && serialPort->isOpen())
        serialPort->close();

    delete serialPort;
}

void PodLive::stop()
{
    state = ERROR;
    sleep(1);
}

bool PodLive::init()
{
    serialPortName=this->find();

    if(serialPortName==QString(""))
    {
        state = ERROR;
        emit error("Unable to locate Live pod.");
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

    int serialPortBaudRate = 250000;
    if (!serialPort->setBaudRate(serialPortBaudRate)   ||
            !serialPort->setDataBits(QSerialPort::Data8)   ||
            !serialPort->setParity(QSerialPort::NoParity)  ||
            !serialPort->setStopBits(QSerialPort::TwoStop) ||
            !serialPort->setFlowControl(QSerialPort::NoFlowControl))
    {
        emit error("Unable to set serial port parameters.");
        state = ERROR;
        return false;
    }
    state = INITIALISED;

    readData.clear();
    //    connect(serialPort, SIGNAL(readyRead()), SLOT(handleReadyRead()));
    connect(serialPort, SIGNAL(error(QSerialPort::SerialPortError)), SLOT(handleError(QSerialPort::SerialPortError)));

    return true;
}

//Same usb vendor and product as PodA
QString PodLive::find()
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


void PodLive::getData( std::vector<ExerciseSet>& exdata )
{

    if (readData.isEmpty() || state != DONE)
        return;

    uint16_t type;
    char *buffer = (char*) readData.data();

    do
    {
        unsigned char *ptr = (unsigned char*)buffer;
        type = *(uint16_t*)ptr;

        if (type == 0) // workout header
        {
            std::vector<ExerciseSet> exercises;
            int lengths=0, totaldistance=0, calories=0;

            if (ptr[2] >=1) //Need to determine how/why this increments
            {
                int Y = 2000+ptr[6]; //see if there's a year high byte.
                int M = ptr[8];
                int D = ptr[10];
                int h = ptr[7]  & 0x7f;
                int m = ptr[9]  & 0x7f;
                int s = ptr[11] & 0x7f;
                QDate date(Y,M,D);
                QTime time(h,m,s);

                int d_m = ptr[15]& 0x7f;
                int d_s = ptr[17]& 0x7f;
                int d_h = ptr[13]& 0x7f; //Check hour location
                QTime t_dur(d_h, d_m, d_s);

                int sets = ptr[16];      //Check >255 sets
                int pool = ptr[14];      //check/grab yards flag

                int id = ptr[12];

                ptr += 20;
                int setnum = 1;

                std::vector<double> times;
                std::vector<int> strokes;

                DEBUG("%02d/%02d/%04d %02d:%02d:%02d %d\n", D,M,Y,h,m,s,sets);
                do
                {
                    uint16_t type =  *(uint16_t*)ptr;

                    DEBUG("Type %04x: ", type);

                    if (type == 0x100)      // length field
                    {
                        double duration;
                        int strks;

                        if ((ptr[3] & 0x80) == 0x80)
                        {
                            duration = (ptr[2] + ((ptr[3] & 0x7f)<<8)) / 8.0;
                            strks = ptr[4];
                            ptr += 6;
                        }
                        else // sometimes time field not complete, attempt to detect and skip.
                        {
                            DEBUG("Incomplete length packet");
                            duration = (((ptr[2] & 0x7f) <<8))/8.0;
                            strks = ptr[3];
                            ptr += 5;
                        }
                        DEBUG("%.2f, %d\n", duration, strks);
                        times.push_back(duration);
                        strokes.push_back(strks);
                    }
                    else if (type == 0x200) // set end
                    {
                        int lens = ptr[2]; // + ((ptr[3]&0x7f)<<8); this isn't high bit
                        int h = ptr[3]&0x7f;
                        int m = ptr[5]&0x7f;
                        int s = ptr[6];
                        QTime dur(h, m, s);

                        //h = ptr[7]&0x7f; //csv only has mm:ss
                        m = ptr[8];
                        s = ptr[9]&0x7f;
                        QTime rest(0,m,s);

                        int cal = ptr[10] + ((ptr[11] & 0x7f)<<8);

                        DEBUG("Lens %d %02d:%02d:%02d\n", lens, h,m,s);

                        ptr += 16;
                        sets--;

                        ExerciseSet set;
                        set.sync = 0;
                        set.user = 1;    // retrieve
                        set.date = date;
                        set.time = time;
                        set.totalduration = t_dur;
                        set.set = setnum++;
                        set.duration = dur;

                        if (id == 0x82) //Chrono
                        {
                            set.type = "Chrono";
                            set.speed = 0;
                            set.strk = 0;
                            set.rate = 0;
                            set.effic = 0;
                        }
                        else if (id == 0x80)
                        {
                            set.type = "SwimHR";

                            set.cal = cal;
                            set.unit = "m";
                            set.pool = pool;

                            lengths += lens;
                            totaldistance += lens*pool;
                            calories += cal;

                            set.dist = lens * pool; // Set data
                            set.lens = lens;

                            //strokes for set:
                            int all_strokes = std::accumulate(strokes.begin(),strokes.end(),0);

                            //For some reason Swimovate goes off the duration not the total seconds:
                            int setsecs = ((dur.hour()*60+dur.minute())*60+dur.second());

                            set.rest = rest;

                            if (lens)
                            {
                                set.speed = 100 * setsecs / set.dist;
                                set.strk = all_strokes / lens;
                                set.effic = ((25 * setsecs / lens) + (25 * set.strk)) / pool;
                                set.rate = (60 * set.strk * lens) / setsecs;
                            }
                            else //Match swimovate data
                            {
                                set.speed = 0;
                                set.strk = all_strokes;
                                set.rate = 0;
                                set.effic = all_strokes;
                            }

                            set.len_time = times;
                            set.len_strokes = strokes;
                        }
                        else //Unknown
                        {
                            set.type = "Unknown";
                            printf("Unknown data id type, please report this.\n");
                        }

                        exercises.push_back(set);

                        times.clear();
                        strokes.clear();
                    }

                } while (sets>0);
            }

            //backfill totals
            std::vector<ExerciseSet>::iterator i;
            for (i=exercises.begin(); i!= exercises.end(); ++i)
            {
                i->lengths = lengths;
                i->totaldistance = totaldistance;
                i->cal = calories;
                exdata.push_back(*i);
            }
        }

        buffer += 0x100; //Walk over packets to next workout start
    } while (buffer < readData.data()+ readData.length()); //Again, track number of packets instead.
}

void PodLive::handleError(QSerialPort::SerialPortError serialPortError)
{
    if (serialPortError == QSerialPort::ReadError)
    {
        state=ERROR;
    }
}

// Captured data packets:
unsigned char d1[] = {0x00, 0x63, 0x63, 0x00, 0x00, 0x00}; //5555 prefixed
unsigned char d5[] = {0x00,0x06,0x80,0x00,0x00,0x00}; //Download start

void display(QByteArray& data)
{
    int i;
    DEBUG("%d :", data.length());
    for (i=0; i < data.length(); ++i)
    {
        DEBUG("%02x ", (unsigned char)data[i]);
        if ((i & 0xff) == 0xff)
        {
            DEBUG("\n");
        }
    }
    DEBUG("\n");
}


namespace
{
QByteArray data;
void read(QSerialPort *serialPort, unsigned long len)
{

    data.clear();
    //wait for buffer to fill
    while (serialPort->waitForReadyRead(100) != false);

    char tmp[266];
    serialPort->read(tmp, len);  //read echo back

    if (serialPort->bytesAvailable())
    {
        serialPort->read(tmp,1); //Skip initial zero byte
        data = serialPort->readAll();
    }
    display(data);
}

void write(QSerialPort *serialPort, unsigned char* data, unsigned long len)
{
    QByteArray out((char*)data,len);
    serialPort->write(out);
    serialPort->waitForBytesWritten(100);
}

// prefix 0x55 packet
void sendandstart(QSerialPort *serialPort, unsigned char* data, int len)
{
    uint32_t crc = crc32a(data,len);

    unsigned char buf[256];
    unsigned char head[]={0x00,0x00,0x55,0x55,0x55,0x55};
    memcpy(buf, head, 6);
    memcpy(&buf[6], data, len);
    memcpy(buf+6+len, &crc, sizeof(uint32_t));

    write(serialPort, buf, 6+len+sizeof(uint32_t));
    read(serialPort, 6+len+sizeof(uint32_t));
}

// prefix 0xff packet
void sendandwait(QSerialPort *serialPort, unsigned char* data, int len)
{
    uint32_t crc = crc32a(data,len);

    unsigned char buf[256];
    unsigned char head[]={0x00,0x00,0xff,0xff,0xff,0xff};
    memcpy(buf, head, 6);
    memcpy(&buf[6], data, len);
    memcpy(buf+6+len, &crc, sizeof(uint32_t));

    write(serialPort, buf, 6+len+sizeof(uint32_t));
    read(serialPort,6+len+sizeof(uint32_t));
}

}; //namespace
bool PodLive::download(QSerialPort *serialPort, QByteArray& readData)
{
    uint32_t req;
    req = *(uint32_t*)data.data();

    DEBUG("Bitflags: %04x\n", req); //bitflags

    unsigned char buf[20]; //6 header, 10 msg, 4 chksum
    unsigned char msg[]={0x00,0x00,0xff,0xff,0xff,0xff,
                         0x01,0x04,
                         0x00,0x00, //dataset number
                         0x00,0x00, 0x00,0x00, 0x00,0x00 };
    memcpy(buf, msg, 16);

    for (int i = 0; i < 0x20; ++i)
    {
        if (req & (1 << i)) // start of dataset marker
        {
            QByteArray workout;
            int missing = -1; // marks uninitialised
            int blockToRequest = i;
            do
            {
                if (blockToRequest != i)
                {
                    if (req & (1 << blockToRequest))
                    {
                        DEBUG("Bad workout %02x, overlapping at %02x", i, blockToRequest);
                        // block already used
                        // ignore
                        // and leave
                        workout.clear();
                        goto done;
                    }
                }

                uint32_t crc, crcin;
                int retry = 4;
                do
                {
                    buf[9]=(blockToRequest & 0x1f);
                    crc = crc32a(&buf[6], 10);
                    memcpy(&buf[16], &crc, sizeof(uint32_t));

                    DEBUG("Request set %02x ", blockToRequest);
                    write(serialPort, buf, 20);
                    read(serialPort, 20);
                    DEBUG("Read %d\n", data.length());

                    crc = crc32a((unsigned char*)data.data(), data.length()-4);
                    crcin = *(uint32_t*)(data.data() + data.length()-4);
                } while (crc != crcin && --retry > 0);

                if (retry == 0)
                {
                    return false; //give up
                }

                workout.append(data.data(), data.length()-4);

                if (missing < 0)
                {
                    const int totalBlocks = data[2];
                    if (totalBlocks == 0)
                    {
                        return false;
                    }
                    missing = totalBlocks;
                }

                --missing;
                blockToRequest = (blockToRequest + 1) & 0x1f;
            }
            while (missing > 0);

            readData.append(workout.data(), workout.length());
        }
        emit progress( i*100/0x20 );
        yieldCurrentThread();
    }

done:
    DEBUG("done\n");
    return true;
}

//
// Begin sync, just use the simple synchronous transfer from qttest for now:
//
void PodLive::run()
{
    if (serialPort && state == INITIALISED )
    {
        emit info("Connect watch.");

        do {
            sendandstart(serialPort, d1, sizeof(d1));
            usleep(50);
            sendandwait(serialPort, d5, sizeof(d5)); //get data to decode.
            usleep(50);

            if (data.length()==0)
                sleep(1);

        } while (state != ERROR && data.length() == 0);

        if (state != ERROR)
        {
            emit info("Transferring.");

            state = TRANSFER;
            if (download(serialPort, readData))
            {
                state = DONE;
                emit info("Complete.");
            }
            else
            {
                state = ERROR;
                emit info("Download error.");
            }
            emit progress( 100 );
        }
    }
}
