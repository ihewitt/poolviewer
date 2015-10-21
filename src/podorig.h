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

#ifndef PODORIG_H
#define PODORIG_H

#include "podbase.h"

// The original pod. Use internal libusb logic to communicate directly.
class PodOrig : public PodBase
{
    Q_OBJECT
public:
    PodOrig();
    ~PodOrig();

    void run();

    virtual bool init();
    virtual void stop();
    virtual void getData(std::vector<ExerciseSet>& data);

signals:
    void info(QString msg);
    void error(QString msg);
    void progress(int progress);
};

#endif // PODORIG_H
