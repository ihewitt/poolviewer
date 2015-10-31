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
