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

///
PodLive::PodLive() : serialPort(NULL)
{
}

PodLive::~PodLive()
{
    if(serialPort != NULL && serialPort->isOpen())
        serialPort->close();

    delete serialPort;
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
    sleep(1);

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

    if (readData.isEmpty())
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

            if (ptr[2] >=2) //1 == chrono, dump for now >2 seems to be swim
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

                ptr += 20;
                int setnum = 1;

                std::vector<double> times;
                std::vector<int> strokes;

                do
                {
                    uint16_t type =  *(uint16_t*)ptr;
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
                            duration = (((ptr[2] & 0x7f) <<8))/8.0;
                            strks = ptr[3];
                            ptr += 5;
                        }
                        times.push_back(duration);
                        strokes.push_back(strks);
                    }
                    else if (type == 0x200) // set end
                    {
                        int lens = ptr[2] + ((ptr[3]&0x7f)<<8);
                        int h = ptr[4];
                        int m = ptr[5]&0x7f;  //add hours
                        int s = ptr[6];
                        int cal = ptr[10] + ((ptr[11] & 0x7f)<<8);

                        QTime dur(h, m, s);
//                        int secs = (h*60 + m)*60 + s;  // TODO find hours.

                        ptr += 16;
                        sets--;

                        ExerciseSet set;
                        set.user = 1;    // retrieve
                        set.date = date;
                        set.time = time;
                        set.type = "SwimHR"; //?
                        set.totalduration = t_dur;
                        set.set = setnum++;
                        set.duration = dur;
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
//                        double all_time = std::accumulate(times.begin(), times.end(), 0);

                        //For some reason Swimovate goes off the duration not the total seconds:
                        int setsecs = ((dur.hour()*60+dur.minute())*60+dur.second());

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
    printf("%d :", data.length());
    for (i=0; i < data.length(); ++i)
    {
        printf("%02x ", (unsigned char)data[i]);
        if ((i & 0xff) == 0xff)
        {
            printf("\n");
        }
    }
    printf("\n");
}


namespace
{
QByteArray data;
void read(QSerialPort *serialPort, unsigned long len)
{

    data.clear();
    //wait for buffer to fill
    while (serialPort->waitForReadyRead(100) != false);

    char tmp[len];
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

void download(QSerialPort *serialPort, QByteArray& readData)
{
    uint32_t req;
    req = *(uint32_t*)data.data();

    printf("Bitflags: %04x\n", req); //bitflags

    unsigned char buf[20]; //6 header, 10 msg, 4 chksum
    unsigned char msg[]={0x00,0x00,0xff,0xff,0xff,0xff,
                         0x01,0x04,
                         0x00,0x00, //dataset number
                         0x00,0x00, 0x00,0x00, 0x00,0x00 };
    memcpy(buf, msg, 16);

    int i;
    for (i=0; i<=0x1f; ++i) // determine upper value
    {
        if (req & 1)
        {
            buf[9]=i;
            uint32_t crc = crc32a(&buf[6], 10);
            memcpy(&buf[16], &crc, sizeof(uint32_t));

            printf("Request set %02x ", i);
            write(serialPort, buf, 20);
            read(serialPort, 20);

            printf("Read %d\n", data.length());

            //check crc.
            readData.append(data.data(), data.length()-4);
        }
        req = req >> 1;

        // Bitfield doesnt seems to match populated datasets so
        // Try to spot packet run ons.

        if (data.length()>5 && data[data.length()-5] != (char)255) // <Improve this logic!
        {
            if ((req & 1) == 0)
            {
                printf("More data, overriding bitmask.\n");
                req |= 1;
            }
        }

        //emit progress( 0x30*100/(1+i) );
    }
    printf("done\n");
}

}; //namespace

//
// Begin sync, just use the simple synchronous transfer from qttest for now:
//
void PodLive::run()
{
    if (serialPort && state == INITIALISED )
    {
        sendandstart(serialPort, d1, sizeof(d1));
        sendandwait(serialPort, d5, sizeof(d5)); //get data to decode.

        state = TRANSFER;
        download(serialPort, readData);
        state = DONE;
        emit progress( 100 );
    }
}
