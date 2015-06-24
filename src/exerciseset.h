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
    int user;                   /**< user number 1..n */
    QDate date;                 /**< date of workout */
    QTime time;                 /**< time of workout begining */
    QString type;               /**< type of watch Swim, SwimHR */
    int pool;                   /**< Pool length */
    QString unit;               /**< Pool length unit m,? */
    QTime totalduration;        /**< Total duration of workout */
    int cal;                    /**< calories (Kcal) used during this workout */
    int lengths;                /**< number of length in this workout */
    int totaldistance;          /**< total distance of this workout (in units) */

    int set;                    /**< Set/lap number 1..n */
    QTime duration;             /**< duration of this set/lap */
    int lens;                   /**< number of length for this set/lap */
    int strk;                   /**< average number of strokes per length for this set/lap */
    int dist;                   /**< distance for this set/lap == e.lens*e.pool  */
    int speed;                  /**< time to cover 100m */
    int effic;                  /**< swolf(ish) for this set/lap. Poolmate uses number of "stroke cycles" i.e. one arm. Garmin uses "stroke count" i.e. both arms */
    int rate;                   /**< strokes per minute */
    QTime rest;                 /**< rest time for this set/lap */

    std::vector<double> len_time; /**< duration of each length in this set/lap in seconds */
    std::vector<int> len_strokes; /**< number of strokes for each length for this set/lap */
    std::vector<QString> len_style;   /**< stroke style for each length for this set/lap */
};

#endif
