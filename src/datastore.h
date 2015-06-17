/*
 * This file is part of PoolViewer
 * Copyright (c) 2011-2012 Ivor Hewitt
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

#ifndef DATASTORE_H
#define DATASTORE_H

#include <vector>
#include "exerciseset.h"

struct Set
{
    int set;
    QTime duration;
    int lens;
    int strk;
    int dist;
    int speed;
    int effic;
    int rate;
    QTime rest;

    std::vector<double> times;
    std::vector<int> strokes;
    std::vector<QString> styles;
};

struct Workout
{
    int id;

    int user;
    QDate date;
    QTime time;
    QString type;
    int pool;
    QString unit;
    QTime totalduration;
    QTime rest;

    int max_eff;
    int avg_eff;
    int min_eff;

    int cal;
    int lengths;
    int totaldistance;

    std::vector<Set> sets;
};


class DataStore
{
public:
    DataStore();
    
    // Locate first exercise id with matching datetime.
    int findExercise( QDateTime dt);
    int findExercise( QDate dt);

    // Insert exercise into datastore, return id
    int add( const std::vector<ExerciseSet>& set) ;

    // Remove exercise with id
    void remove(int id);
    void removeSet( int wid, int sid );

    // Remove all exercises at date
    void remove( QDateTime dt );

    //move?
    void setFile(const QString &_filename) { filename=_filename;}
    const QString& getFile() { return filename;}

    //once loaded flag any manipulation.
    bool hasChanged() { return changed;}

    //
    bool load();
    bool save();

    const std::vector<Workout>& Workouts() const;

private:
    //assign id to each workout
    int counter;

    // Exercise sets must be sorted by time
    mutable std::vector<Workout> workouts;

    bool changed;
    mutable bool sorted;
    QString filename;
};

#endif
