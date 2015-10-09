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

#ifndef PODBASE_H
#define PODBASE_H

#include <QThread>
#include <QtSerialPort/QtSerialPort>
#include <inttypes.h>
#include "exerciseset.h"

uint32_t crc32a(unsigned char *message, int len);

class PodBase : public QThread
{
Q_OBJECT

public:
    PodBase() {}
    ~PodBase() {}

    //remove state machine
    enum State {
        STARTUP,
        ERROR,
        INITIALISED,
        READY,
        TRANSFER,
        DONE
    };
    State state;

    virtual void getData(std::vector<ExerciseSet>& data) = 0;
};

#endif //PODBASE_H
