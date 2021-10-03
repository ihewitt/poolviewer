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

void synchroniseSet(Set & set, const Workout & workout)
{
    set.dist = workout.pool * set.lens;
    const int setsecs = set.duration.msecsSinceStartOfDay() / 1000;
    set.speed = 100 * setsecs / set.dist;
    set.effic = ((25 * setsecs / set.lens) + (25 * set.strk)) / workout.pool;
    set.rate = (60 * set.strk * set.lens) / setsecs;
    set.strk = std::accumulate(set.strokes.begin(), set.strokes.end(), 0) / set.lens;
}

QString formatSpeed(const int speed, const bool asMinuteAndSeconds)
{
    if (asMinuteAndSeconds)
    {
        QTime time = QTime(0, 0).addSecs(speed);
        return time.toString("mm:ss");
    }
    else
    {
        return QString::number(speed);
    }
}

bool getFastestSubset(const Set & set, const int pool, const int targetDistance,
                      double & duration, double & range, int & distance)
{
    if (pool * set.lens < targetDistance)
    {
        return false;
    }
    else
    {
        // this is the number of lengths to get above or equal the desired distance
        const int numberOfLengths = (targetDistance + pool - 1) / pool; // round up
        distance = numberOfLengths * pool;

        if ((int)set.times.size() != set.lens)
        {
            // Non poolmate live, try to use set duration.
            // Since we're taking distances >= numberOfLengths which should be slower, assume we can
            // just divide to get the closes time for shorter distance.
            duration = (QTime(0, 0).secsTo(set.duration) * numberOfLengths / set.lens);
            range = 0.0;
        }
        else
        {
            // Locate fastest window
            double min = std::numeric_limits<double>::max();
            for (int j = 0; j <= (int)set.times.size() - numberOfLengths; ++j)
            {
                double cur = 0.0;
                double fastest = std::numeric_limits<double>::max();
                double slowest = -fastest;
                for (int i = j; i < j + numberOfLengths; ++i)
                {
                    const double time = set.times[i];
                    fastest = std::min(fastest, time);
                    slowest = std::max(slowest, time);
                    cur += time;
                }
                if (cur < min)
                {
                    min = cur;
                    range = slowest - fastest;
                }
            }
            duration = min;
        }
        const double adjustment = double(targetDistance) / double(numberOfLengths * pool);
        duration *= adjustment;
        return true;
    }
}
