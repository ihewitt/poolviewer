/*
 * This file is part of PoolViewer
 * Copyright (c) 2015 Ivor Hewitt
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

#ifndef USBLINK_H
#define USBLINK_H

#include <QThread>
#include <QtSerialPort/QtSerialPort>

class UsbBase : public QThread
{
Q_OBJECT

public:
    UsbBase() {}
    ~UsbBase() {}

    enum State {
        STARTUP,
        ERROR,
        INITIALISED,
        READY,
        TRANSFER,
        DONE
    };
    State state;

    virtual unsigned char* data() =0;
    virtual int len() =0;
};

// Implementation for the poolmate "Pod-A"
// this is supported by the native linux serial interface so use
// QSerialPort instead

class UsbA : public UsbBase
{
Q_OBJECT
public:
    UsbA();
    ~UsbA();

    void run();

    unsigned char* data();
    int len();

signals:
    void info(QString msg);
    void error(QString msg);
    void progress(int progress);

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);

private:
    QString      find();
    QSerialPort *serialPort;
    QString      serialPortName;
    QByteArray   readData;
};

//
class UsbOrig : public UsbBase
{
Q_OBJECT
public:
    UsbOrig();
    ~UsbOrig();

    void run();

    unsigned char* data();
    int len();

signals:
    void info(QString msg);
    void error(QString msg);
    void progress(int progress);
};



#endif // USBLINK_H

