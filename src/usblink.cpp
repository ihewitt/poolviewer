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

extern "C"
{
#include "poolmate.h"
};

#include "logging.h"
#include "usblink.h"

// TODO reorganise properly and get rid of our horrible state machine now we know
// what we need to do.

UsbOrig::UsbOrig()
{
    state = STARTUP;
}

UsbOrig::~UsbOrig()
{
    if (state != STARTUP)
        poolmate_cleanup();
}

void UsbOrig::run()
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

                emit progress( count );
            }
            count = poolmate_len();
            emit progress( count );

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

unsigned char* UsbOrig::data()
{
    return poolmate_data();
}

int UsbOrig::len()
{
    return poolmate_len();
}

// Type A pod interface

UsbA::UsbA()
{
    state = STARTUP;

    serialPort=NULL;
    serialPortName=this->find();

    if(serialPortName==QString(""))
    {
        state = ERROR;
        emit error("Unable to locate TypeA pod.");
        return;
    }

    serialPort= new QSerialPort();
    serialPort->setPortName(serialPortName);

    //m_standardOutput= &out;
    if (!serialPort->open(QIODevice::ReadWrite)) {
        emit error("Unable to open serial device, check permissions.");
        state = ERROR;
        return;
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
        return;
    }

    readData.clear();
    connect(serialPort, SIGNAL(readyRead()), SLOT(handleReadyRead()));
    connect(serialPort, SIGNAL(error(QSerialPort::SerialPortError)), SLOT(handleError(QSerialPort::SerialPortError)));
}

QString UsbA::find()
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

UsbA::~UsbA()
{
    if(serialPort != NULL && serialPort->isOpen())
        serialPort->close();

    delete serialPort;
}

void UsbA::run()
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

                emit progress( count );

                yieldCurrentThread();
            }

            count = len();
            emit progress( count );

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

void UsbA::handleReadyRead()
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

unsigned char* UsbA::data()
{
    if (readData.isEmpty())
        return NULL;
    else
        return (unsigned char*) readData.data();
}

int UsbA::len()
{
    return readData.length();
}


void UsbA::handleError(QSerialPort::SerialPortError serialPortError)
{
    if (serialPortError == QSerialPort::ReadError)
    {
        state=ERROR;
    }
}
