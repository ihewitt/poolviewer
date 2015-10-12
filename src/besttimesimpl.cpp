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
    void addSet(const Set & set, const Workout & workout, const int numberOfLanes, QTableWidget * table)
    {
        const int distance = workout.pool * numberOfLanes;
        QTime duration(0, 0);

        if (set.times.size() != set.lens)
        {
            // error, ignore set
            return;
        }

        for (size_t i = 0; i < numberOfLanes; ++i)
        {
            duration = duration.addMSecs(set.times[i] * 1000.0);
        }

        const double speed = duration.msecsSinceStartOfDay() / distance / 10.0;
        const double total = workout.pool * set.lens;

        const int row = table->rowCount();
        table->setRowCount(row + 1);

        QTableWidgetItem * dateItem = createReadOnlyItem(QVariant(workout.date));
        table->setItem(row, 0, dateItem);

        QTableWidgetItem * timeItem = createReadOnlyItem(QVariant(workout.time));
        table->setItem(row, 1, timeItem);

        QTableWidgetItem * poolItem = createReadOnlyItem(QVariant(workout.pool));
        table->setItem(row, 2, poolItem);

        QTableWidgetItem * distanceItem = createReadOnlyItem(QVariant(distance));
        table->setItem(row, 3, distanceItem);

        QTableWidgetItem * durationItem = createReadOnlyItem(QVariant(duration.toString()));
        table->setItem(row, 4, durationItem);

        QTableWidgetItem * speedItem = createReadOnlyItem(QVariant(speed));
        table->setItem(row, 5, speedItem);

        QTableWidgetItem * totalItem = createReadOnlyItem(QVariant(total));
        table->setItem(row, 6, totalItem);
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
    timesTable->clearContents();
    timesTable->setRowCount(0);

    // otherwise the rows move around while they are added
    // and we set the item in the wrong place
    timesTable->setSortingEnabled(false);

    const QString distStr = distanceBox->currentText();
    bool ok = false;
    const uint distance = distStr.toUInt(&ok);
    if (!ok || distance == 0)
    {
        return;
    }

    const std::vector<Workout>& workouts = ds->Workouts();

    for (size_t i = 0; i < workouts.size(); ++i)
    {
        const Workout & w = workouts[i];

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
        const uint numberOfLanes = (distance + pool - 1) / pool; // round up

        for (size_t j = 0; j < w.sets.size(); ++j)
        {
            const Set & set = w.sets[j];
            if (set.lens < numberOfLanes)
            {
                // not enough in this set
                continue;
            }

            addSet(set, w, numberOfLanes, timesTable);
        }
    }

    timesTable->resizeColumnsToContents();
    timesTable->setSortingEnabled(true);
}
