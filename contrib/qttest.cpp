// Build:
// g++ -fPIC qttest.cpp -I /usr/include/qt5 -l Qt5SerialPort -l Qt5Core

// Sample serial code for downloading/dumping data from a PoolMate Live
// Very little (none) validation, checking, or sanity logic.

#include <unistd.h>
#include <iostream>
#include <inttypes.h>
#include <QtSerialPort/QtSerialPort>

//CRC32 helper function for PoolmateLive protocol
// Non-optimised crc32 calc.
// Doesn't need to be fast so leave readable.
uint32_t crc32a(char *message, int len)
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

// Locate vendor and product id for serial interface.
QString find()
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

// Captured data packets:
char d1[] = {0x00, 0x63, 0x63, 0x00, 0x00, 0x00}; //5555 prefixed
char d2[] = {0x00, 0x03, 0x00, 0x00, 0x00, 0x00}; // d2 again
//returns 00 42 08 00 00 30 FE 37 5B

char d2b[] = {0x01,0x07,0x16,0x05,0x0f,0x00,0x1c,0x01,0x0b,0x00};
//returns single 00

char d3[] = {0x00, 0x01, 0x00, 0x00, 0x08, 0x08};
 //returns 00 2D 01 00 00 4A 86 B8 8A

char d4[] = {0x00, 0x03, 0x04, 0x00, 0x00, 0x00};
//returns 00 33 0B 00 00 09 6A DD 49

char d5[] = {0x00,0x06,0x80,0x00,0x00,0x00}; //Download start
//00 01 00 00 00 00 00 00 00 00 etc
//00 01 2D 95 AA 00 00 00 00 00 etc  45, 149, 170

char d6[] = {0x01, 0x04, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00}; // workout request #0
char d7[] = {0x01, 0x04, 0x00, 0x01, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00}; // #1
char d8[] = {0x01, 0x04, 0x00, 0x08, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00}; // #8


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

QByteArray data;
void read(QSerialPort *serialPort, unsigned long len)
{

    data.clear();
    //wait for buffer to fill
    while (serialPort->waitForReadyRead(100) != false);

    char tmp[len];
    serialPort->read(tmp, len); //read echo back

    if (serialPort->bytesAvailable())
    {
        // swallow initial zero
        serialPort->read(tmp, 1);
        data = serialPort->readAll();
    }
}

void write(QSerialPort *serialPort, char* data, unsigned long len)
{
    QByteArray out(data,len);
    serialPort->write(out);
    serialPort->waitForBytesWritten(100);
}

void init(QSerialPort* serialPort)
{
// Protocol used by windows app:
    serialPort->setBaudRate(250000);
    serialPort->setDataBits(QSerialPort::Data8);
    serialPort->setParity(QSerialPort::NoParity);
    serialPort->setStopBits(QSerialPort::TwoStop);
//    serialPort->setFlowControl(QSerialPort::SoftwareControl); // xon/off seems to break with some packets.
    serialPort->setFlowControl(QSerialPort::NoFlowControl);
}

// prefix 0x55 packet
void sendandstart(QSerialPort *serialPort, char*data, int len)
{
    uint32_t crc = crc32a(data,len);

    char buf[256];
    char head[]={0x00,0x00,0x55,0x55,0x55,0x55};
    memcpy(buf, head, 6);
    memcpy(&buf[6], data, len);
    memcpy(buf+6+len, &crc, sizeof(uint32_t));

    write(serialPort, buf, 6+len+sizeof(uint32_t));
    read(serialPort, 6+len+sizeof(uint32_t));
}

// prefix 0xff packet
void sendandwait(QSerialPort *serialPort, char*data, int len)
{
    uint32_t crc = crc32a(data,len);

    char buf[256];
    char head[]={0x00,0x00,0xff,0xff,0xff,0xff};
    memcpy(buf, head, 6);
    memcpy(&buf[6], data, len);
    memcpy(buf+6+len, &crc, sizeof(uint32_t));

    write(serialPort, buf, 6+len+sizeof(uint32_t));
    read(serialPort,6+len+sizeof(uint32_t));
}

unsigned char * decode_sets(unsigned char* data, int sets)
{
    do
    {
        uint16_t type =  *(uint16_t*)data;
        if (type == 0x100)         //length field
        {
            double time;
            int strks;

            if ((data[3] & 0x80) == 0x80)
            {
                time = (data[2] + ((data[3] & 0x7f)<<8)) / 8.0;
                strks = data[4];
                data += 6;
            }
            else // sometimes time field not complete, attempt to detect and skip.
            {
                printf("        Not complete!\n");
                time = (((data[2] & 0x7f) <<8))/8.0;
                strks = data[3];
                data += 5;
            }

            printf("        Len time %2.2f, Strks %d\n", time, strks);
        }
        else if (type == 0x200)    //set end
        {
            int lens = data[2];
            int m = data[5]&0x7f;  //add hours
            int s = data[6];
            int cal = data[10];    //add high byte.

            data += 16;

            sets--;
            printf("Set lengths %d, Time: %02d:%02d\n", lens, m,s);
        }
    } while(sets>0);

    return data; // point to next field
}

char * decode_wrk(char* data)
{
    if (data[2] == 1)
    {
        printf("Chrono\n");
        return data+256;
    }
    else if (data[2] == 2 ||
	     data[2] == 3 ||
	     data[2] == 4)   //Seems to start either 2 or 3 for a swim session for some reason.
    {
        printf("Swim: %d\n", data[2]);

        int Y = data[6]; //see if there's a year high byte.
        int h = data[7]  & 0x7f;
        int M = data[8];
        int m = data[9]  & 0x7f;
        int d = data[10];
        int s = data[11] & 0x7f;

        int pool = data[14];      //check/grab yards flag
        int d_m = data[15]& 0x7f;
        int sets = data[16];      //Check >255 sets
        int d_s = data[17]& 0x7f;
        int d_h = data[13]& 0x7f; //Check hour location

        printf("Workout: %02d/%02d/%02d %02d:%02d:%02d\n", d, M, Y+2000, h, m, s);
        printf("   Pool: %dm, Sets %d, Dur: %02d:%02d:%02d\n", pool, sets, d_h, d_m, d_s);
        data = (char*)decode_sets((unsigned char*)data+20, sets);
    }
    else
    {
        printf("Unknown type: %d\n", data[2]);
    }
    return data;
}

void download(QSerialPort *serialPort)
{
    if (data.length() == 0)
    {
        printf("Watch attached?\n");
        return;
    }
    uint32_t req;
    req = *(uint32_t*)data.data();

    printf("Bitflags: %04x\n", req); //bitflags

    char buf[20]; //6 header, 10 msg, 4 chksum
    char msg[]={0x00,0x00,0xff,0xff,0xff,0xff,
                0x01,0x04,
                0x00,0x00, //dataset number
                0x00,0x00, 0x00,0x00, 0x00,0x00 };
    memcpy(buf, msg, 16);

    QByteArray buffer;

    int i;
    for (i=0; i<=0x30; ++i) // determine upper value
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
            buffer.append(data.data(), data.length()-4);
        }
        req = req >> 1;

        // Bitfield doesnt seems to match populated datasets so
        // Try to spot packet run ons.
        if (data[data.length()-5] != (char)255) // <Improve this logic
	{
            if ((req & 1) == 0)
            {
                printf("More data, overriding bitmask.\n");
                req |= 1;
            }
	}
    }
    printf("Data buffer:\n");
    display(buffer);

    uint16_t type;
    char *ptr = buffer.data();
    do
    {
        type = *(uint16_t*)ptr;

        if (type == 0) // workout header
        {
	    decode_wrk(ptr);
        }
	ptr += 0x100; //Walk over packets to next workout start
    } while (ptr < buffer.data()+ buffer.length()); //Again, track number of packets instead.

    printf("done\n");
}


int main()
{
    QSerialPort *serialPort = new QSerialPort();
    QString serialPortName = find();

    serialPort->setPortName(serialPortName);
    if (!serialPort->open(QIODevice::ReadWrite)) {
        return 1;
    }

    init(serialPort);
    sleep(1);

    sendandstart(serialPort, d1, sizeof(d1));

// Ignore these initial requests for now.
//    sendandwait(serialPort, d2, sizeof(d2));
//    sendandwait(serialPort, d3, sizeof(d3));
//    sendandwait(serialPort, d4, sizeof(d4));

    sendandwait(serialPort, d5, sizeof(d5)); //get data to decode.
    printf("Data header:\n");
    display(data);

//pulldown all data and dump
    download(serialPort);

    serialPort->close();
    delete serialPort;
}
