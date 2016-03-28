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

#include "besttimesimpl.h"
#include "datastore.h"
#include "utilities.h"

namespace
{
    void addSet(const Set & set, const Workout & workout, const int numberOfLanes, double & bestSpeed, QTableWidget * table)
    {
        const int distance = workout.pool * numberOfLanes;
        QTime duration(0, 0);
        double range = 0.0; // time range per length: slowest - fastest

        if ((int)set.times.size() != set.lens)
        {
            // Non poolmate live, try to use set duration.
            // Since we're taking distances >= numberOfLanes which should be slower, assume we can
            // just divide to get the closes time for shorter distance.
            duration = duration.addMSecs( (QTime(0,0).msecsTo(set.duration) * numberOfLanes / set.lens ));
        }
        else
        {
            // Locate fastest window
            double min = 0.0;
            for (int j = 0; j <= (int)set.times.size()-numberOfLanes; ++j)
            {
                double cur = 0.0;
                double fastest = std::numeric_limits<double>::max();
                double slowest = -fastest;
                for (int i = j; i < j + numberOfLanes; ++i)
                {
                    const double time = set.times[i];
                    fastest = std::min(fastest, time);
                    slowest = std::max(slowest, time);
                    cur += time;
                }
                if (cur < min || min == 0.0)
                {
                    min = cur;
                    range = slowest - fastest;
                }
            }
            duration = duration.addMSecs(min * 1000.0);
        }

        const double speed = duration.msecsSinceStartOfDay() / distance / 10.0;
        const double total = workout.pool * set.lens;

        const int row = table->rowCount();
        table->setRowCount(row + 1);

        QTableWidgetItem * dateItem = createTableWidgetItem(QVariant(workout.date));
        table->setItem(row, 0, dateItem);

        QTableWidgetItem * timeItem = createTableWidgetItem(QVariant(workout.time));
        table->setItem(row, 1, timeItem);

        QTableWidgetItem * poolItem = createTableWidgetItem(QVariant(workout.pool));
        table->setItem(row, 2, poolItem);

        QTableWidgetItem * distanceItem = createTableWidgetItem(QVariant(distance));
        table->setItem(row, 3, distanceItem);

        QTableWidgetItem * durationItem = createTableWidgetItem(QVariant(duration.toString()));
        table->setItem(row, 4, durationItem);

        QTableWidgetItem * speedItem = createTableWidgetItem(QVariant(speed));
        table->setItem(row, 5, speedItem);

        QTableWidgetItem * totalItem = createTableWidgetItem(QVariant(total));
        table->setItem(row, 6, totalItem);

        QTableWidgetItem * rangeItem = createTableWidgetItem(QVariant(range));
        table->setItem(row, 7, rangeItem);

        // Store in column 0 whether this has been a new best time
        // use speed as the time could be for a longer distance if lane does not divide exactly the distance
        const bool record = speed <= bestSpeed;
        if (record)
        {
            bestSpeed = speed;
        }

        dateItem->setData(NEW_BEST_TIME, QVariant(record));
    }
}

BestTimesImpl::BestTimesImpl(QWidget *parent) :
    QDialog(parent), ds(NULL)
{
    setupUi(this);

    // not done in on_calculateButton_clicked() not to override user choice

    // sort by speed as the ditance might be different
    // (if number of lanes is not integer)
    timesTable->sortItems(5);
}

void BestTimesImpl::setDataStore(const DataStore *_ds)
{
    ds = _ds;
}

void BestTimesImpl::on_calculateButton_clicked()
{
    enum period
    {
        ALL = 0,
        THISMONTH,
        THISYEAR,
        LASTMONTH,
        LASTYEAR
    };

    timesTable->clearContents();
    timesTable->setRowCount(0);

    // otherwise the rows move around while they are added
    // and we set the item in the wrong place
    timesTable->setSortingEnabled(false);

    int id = periodBox->currentIndex();

    const QString distStr = distanceBox->currentText();
    bool ok = false;
    const uint distance = distStr.toUInt(&ok);
    if (!ok || distance == 0)
    {
        return;
    }

    double bestSpeed = std::numeric_limits<double>::max();

    const std::vector<Workout>& workouts = ds->Workouts();

    for (size_t i = 0; i < workouts.size(); ++i)
    {
        const Workout & w = workouts[i];

        switch(id)
        {
        case ALL:
            break;

        case THISMONTH:
        {
            QDate now = QDate::currentDate();
            if (w.date.month() != now.month() ||
                    w.date.year() != now.year())
                continue;
            break;
        }

        case THISYEAR:
        {
            QDate now = QDate::currentDate();
            if (w.date.year() != now.year())
                continue;
            break;
        }

        case LASTMONTH:
        {
            QDate then = QDate::currentDate().addMonths(-1);
            if (w.date.month() != then.month() ||
                    w.date.year() != then.year())
                continue;
            break;
        }

        case LASTYEAR:
        {
            QDate then = QDate::currentDate().addYears(-1);
            if (w.date.year() != then.year())
                continue;
            break;
        }
        }

        if (w.type != "Swim" && w.type != "SwimHR")
        {
            continue;
        }

        const int pool = w.pool;

        if (pool <= 0)
        {
            continue;
        }

        // this is the number of lanes to get above or equal the desired distance
        const int numberOfLanes = (distance + pool - 1) / pool; // round up

        for (size_t j = 0; j < w.sets.size(); ++j)
        {
            const Set & set = w.sets[j];
            if (set.lens < numberOfLanes)
            {
                // not enough in this set
                continue;
            }

            addSet(set, w, numberOfLanes, bestSpeed, timesTable);
        }
    }

    // ensure the state of the "record progression" is correct
    on_progressionBox_clicked();
    timesTable->resizeColumnsToContents();
    timesTable->setSortingEnabled(true);
}

void BestTimesImpl::on_progressionBox_clicked()
{
    const bool checked = progressionBox->checkState() == Qt::Checked;
    const size_t numberOfTimes = timesTable->rowCount();
    if (checked)
    {
        for (size_t i = 0; i < numberOfTimes; ++i)
        {
            const QTableWidgetItem * record_item = timesTable->item(i, 0);
            const bool record = record_item->data(NEW_BEST_TIME).value<bool>();
            timesTable->setRowHidden(i, !record);
        }
    }
    else
    {
        for (size_t i = 0; i < numberOfTimes; ++i)
        {
            timesTable->setRowHidden(i, false);
        }
    }
}
