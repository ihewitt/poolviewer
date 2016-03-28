/*
 * This file is part of PoolViewer
 * Copyright (c) 2015 Andrea Odetti
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

#include "utilities.h"

#include <QVariant>
#include <QTableWidgetItem>
#include <cmath>

#include "datastore.h"

const int SET_ID            = Qt::UserRole + 1;
const int LENGTH_ID         = Qt::UserRole + 2;
const int NEW_BEST_TIME     = Qt::UserRole + 3;

QTableWidgetItem * createTableWidgetItem(const QVariant & content)
{
    QTableWidgetItem * item = new QTableWidgetItem();
    item->setFlags(item->flags() ^ Qt::ItemIsEditable);
    item->setData(Qt::DisplayRole, content);
    item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

    return item;
}

// sums the duration of all the lanes
QTime getActualSwimTime(const Set & set)
{
    QTime duration(0, 0);

    for (size_t i = 0; i < set.times.size(); ++i)
    {
        duration = duration.addMSecs(set.times[i] * 1000);
    }
    return duration;
}

// mimic watch that rounds to 8th of a second
double roundTo8thSecond(double value)
{
    const double result = round(value * 8.0) / 8.0;
    return result;
}

// synchronise workout
void synchroniseWorkout(Workout & workout)
{
    const size_t numberOfSets = workout.sets.size();

    int lengths = 0;
    QTime rest;
    QTime totalDuration(0, 0);
    for (size_t i = 0; i < numberOfSets; ++i)
    {
        const Set & set = workout.sets[i];
        lengths += set.lens;
        if (rest.isValid())
        {
            rest = rest.addMSecs(set.rest.msecsSinceStartOfDay());
        }
        else
        {
            // if the sets do not have rest
            // we keep the existing rest on the workout
            // (see end of setsToWorkouts() in datastore.cpp)
            if (set.rest.isValid())
            {
                rest = set.rest;
            }
        }
        totalDuration = totalDuration.addMSecs(set.duration.msecsSinceStartOfDay());
    }

    // always update lenghts and total
    workout.lengths = lengths;
    workout.totaldistance = lengths * workout.pool;

    if (rest.isValid())
    {
        workout.rest = rest;
        // otherwise leave it unchanged
    }

    // use the actual rest to compute total duration
    totalDuration = totalDuration.addMSecs(workout.rest.msecsSinceStartOfDay());
    workout.totalduration = totalDuration;
}
