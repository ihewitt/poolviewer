/*
 * This file is part of PoolViewer
 * Copyright (c) 2011 Ivor Hewitt
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
#ifndef EXERCISESET_H
#define EXERCISESET_H

#include <QDate>

// File format for CSV storage
struct ExerciseSet
{
    int user;
    QDate date;
    QTime time;
    QString type;
    int pool;
    QString unit;
    QTime totalduration;
    int cal;
    int lengths;
    int totaldistance;

    int set;
    QTime duration;
    int lens;
    int strk;
    int dist;
    int speed;
    int effic;
    int rate;
    QTime rest;

    std::vector<double> len_time;
    std::vector<int> len_strokes;
};

#endif
