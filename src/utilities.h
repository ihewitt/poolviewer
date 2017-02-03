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

#ifndef UTILITIES_H
#define UTILITIES_H

#include <QTime>

class QTableWidgetItem;
class QVariant;
struct Set;
struct Workout;

// IDs used in the Qt::UserRole
extern const int SET_ID;
extern const int LENGTH_ID;
extern const int NEW_BEST_TIME;

// ReadOnly, horizontal Right aligned, non editable
QTableWidgetItem * createTableWidgetItem(const QVariant & content);

// sums the duration of all the lanes
QTime getActualSwimTime(const Set & set);

// mimic watch that rounds to 8th of a second
double roundTo8thSecond(double value);

// synchronise workout
void synchroniseWorkout(Workout & workout);

#endif // UTILITIES_H
