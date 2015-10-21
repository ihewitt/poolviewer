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

#include "logging.h"
#include "poda.h"

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
        !serialPort->setStopBits(QSerialPort::TwoStop)||
        !serialPort->setFlowControl(QSerialPort::NoFlowControl))
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

void PodA::stop()
{
    state = DONE;
    sleep(1);
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

                emit progress( count*100/4096 );

                yieldCurrentThread();
            }

            count = len();
            emit progress( count*100/4096 );

            if (count < 4196)
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
    state = STARTUP;
}

void PodA::handleReadyRead()
{
    readData.append(serialPort->readAll());
    int len = readData.length();

    if(len>0)
    {
        this->state=TRANSFER;
    }

    if ( readData.length() > 4196)
    {
        this->state = ERROR;
        return;
    }

    if ( readData.length() == 4196)
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

