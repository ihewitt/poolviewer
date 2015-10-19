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

#ifndef PODLINK_H
#define PODLINK_H

#include "podbase.h"

// Implementation for the poolmate "Pod-A"
// this is supported by the native linux serial interface so use
// QSerialPort instead

class PodA : public PodBase
{
Q_OBJECT
public:
    PodA();
    ~PodA();

    void run();

    virtual bool init();
    virtual void getData(std::vector<ExerciseSet>& data);

signals:
    void info(QString msg);
    void error(QString msg);
    void progress(int progress);

private slots:
    void handleReadyRead();
    void handleError(QSerialPort::SerialPortError error);

private:
    int          len();
    QString      find();
    QSerialPort *serialPort;
    QString      serialPortName;
    QByteArray   readData;
};

// The original pod. Use internal libusb logic to communicate directly.
class PodOrig : public PodBase
{
Q_OBJECT
public:
    PodOrig();
    ~PodOrig();

    void run();

    virtual bool init();
    virtual void getData(std::vector<ExerciseSet>& data);

signals:
    void info(QString msg);
    void error(QString msg);
    void progress(int progress);
};


#endif // PODLINK_H

