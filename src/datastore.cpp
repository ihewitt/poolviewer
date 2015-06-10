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

#include <QStringList>
#include <QTextStream>
#include <QFile>
#include <QDateTime>

#include <stdio.h>

#include <algorithm>
#include "datastore.h"

#include "exerciseset.h"


// Stick to same file format as poolmate app for now so we can share files
bool SaveCSV( const std::string & name, std::vector<ExerciseSet>& exercises )
{
    QFile file(name.c_str());
    if (file.open(QIODevice::WriteOnly|QIODevice::Text))
    {
        QTextStream out(&file);

//New format
        out << "User Number,Date,Time,Type,Pool Length,,Duration,Calories,Total Laps,Total Distance,Set Number,Set Duration,Average Strokes,Distance,Speed,Efficiency,Stroke Rate,,,,,,Watch Version,Status,Notes\n";

        std::vector<ExerciseSet>::iterator i;
        
        for (i=exercises.begin(); i != exercises.end(); ++i)
        {
            out << i->user << ","
                << i->date.toString("dd/MM/yyyy") << ","
                << i->time.toString("hh:mm:00") << ",";
            if (i->type == "Swim")
            {
                out << i->type << ","
                    << i->pool << ","
                    << i->unit << ","
                    << i->totalduration.toString("hh:mm:ss") << ","
                    << i->cal << ","
                    << i->lengths << ","
                    << i->totaldistance << ","
                    << i->set << ","
                    << i->duration.toString("hh:mm:ss") << ","
                    << i->strk << ","
                    << i->lens << ","
                    << i->speed << ","
                    << i->effic << ","
                    << i->rate << ",Free,";
                out << ",,,,210,,,\n";
            }
//1,31/3/2015,06:38:12,SwimHR,25,,00:32:38,397,52,1300,1,00:06:19,12,12,126,44,22,Free,,,,,0,New,00:32:38,,STARTOFLAPDATA,0,0,0,3908.923,00:14,SwimHR,11,-1,28,12,31,13,31.125,13,31.625,13,31.25,13,31.125,13,29.5,11,33.375,13,30.375,13,31.75,12,33.125,13,33,13
            else if (i->type == "SwimHR")
            {

            }
            else
            {
                out << i->type << ","
                    << ",," 
                    << i->totalduration.toString("hh:mm:ss") << ","
                    << ",,,"
                    << i->set << ","
                    << i->duration.toString("hh:mm:ss") << ","
                    << ",,,,,,";
                out << ",,,,210,,,\n";
            }
        }
    }
    return true;
}

//
bool ReadCSV( const std::string & name, std::vector<ExerciseSet>& dst )
{
// Simplistic csv format reading
    QFile file(name.c_str());
    if (file.open(QIODevice::ReadOnly))
    {
        bool oldformat = false;
        QString head = file.readLine(); //skip header
        if (head.contains("LogDate"))
        {
            oldformat = true;
        }

        while (!file.atEnd())
        {
            QString line = file.readLine();
            QStringList strings = line.split(",");

            if (strings.count())
            {
                ExerciseSet e;

                e.user = strings.value(0).toInt();
                e.date = QDate::fromString( strings.value(1), QString("dd/MM/yyyy"));
                e.time = QTime::fromString( strings.value(2));

                e.type = strings.value(3);

                if (e.type == "Swim" || e.type == "SwimHR")
                {
                    e.pool = strings.value(4).toInt();
                    e.unit = strings.value(5);
                    e.totalduration = QTime::fromString(strings.value(6));
                    e.cal = strings.value(7).toInt();
                    e.lengths = strings.value(8).toInt();
                    e.totaldistance = strings.value(9).toInt();
                    e.set = strings.value(10).toInt();
                    e.duration = QTime::fromString( strings.value(11));
                    e.strk = strings.value(12).toInt();
                    e.lens  = strings.value(13).toInt();

                    if (oldformat && e.lens && e.pool) //if we've saved as an oldformat file update to new
                    {
                        e.lens = strings.value(13).toInt() / e.pool;
                    }
                    e.dist = e.lens * e.pool;
                    e.speed  = strings.value(14).toInt();
                    e.effic = strings.value(15).toInt();
                    e.rate  = strings.value(16).toInt();
                }

//1,31/3/2015,06:38:12,SwimHR,25,,00:32:38,397,52,1300,1,00:06:19,12,12,126,44,22,Free,,,,,0,
//New,00:32:38,,STARTOFLAPDATA,
//0,0,0,3908.923,00:14,SwimHR,
//11,-1,28,12,31,13,31.125,13,31.625,13,31.25,13,31.125,13,29.5,11,33.375,13,30.375,13,31.75,12,33.125,13,33,13
//Ignore HR part of data for now.

                if (e.type == "SwimHR") // Read length data
                {
                    double num = strings.value(30).toDouble(); //no idea what this is
                    e.rest = QTime::fromString( strings.value(31));

                    // we already know the number of lengths.
                    int l;
                    for (l = 0; l <e.lens; ++l)
                    {
                        double time = strings.value(35+l*2).toDouble();
                        int strk = strings.value(36+l*2).toInt();

                        e.len_time.push_back(time);
                        e.len_strokes.push_back(strk);
                    }
                }
                dst.push_back(e);
            }
        }
        file.close();

    }   
    return true;
}



DataStore::DataStore()
{
    //TODO always assume changed
    counter=0;
    sorted=false;
    changed=true;
}

namespace {
    // import
    void setsToWorkouts( const std::vector<ExerciseSet>& sets,
                         std::vector<Workout>& workouts)
    {
        std::vector<ExerciseSet>::const_iterator i;
        
        for (i=sets.begin(); i != sets.end();)
        {
            Workout wrk;
            wrk.user = i->user;
            wrk.date = i->date;
            wrk.time = i->time;
            wrk.type = i->type;
            wrk.pool = i->pool;
            wrk.unit = i->unit;
            wrk.totalduration = i->totalduration;
            wrk.cal = i->cal;
            wrk.lengths = i->lengths;
            wrk.totaldistance = i->totaldistance;

            QDateTime td(i->date, i->time);
            std::vector<ExerciseSet>::const_iterator j;
            j=i;
            int n=0;
            QTime rest;
            int total=0;

            int min_effic=999;
            int avg_effic=0;
            int max_effic=0;

            while( j != sets.end() &&
                   td == QDateTime(j->date, j->time) )
            {
                Set set;
                set.set = ++n;
                total += QTime(0,0,0).secsTo(j->duration);

                set.duration = j->duration;
                set.lens = j->lens;
                set.strk = j->strk;
                set.dist = j->dist;
                set.speed = j->speed;
                set.effic = j->effic;
                set.rate = j->rate;
                set.rest = j->rest;

                set.times = j->len_time;
                set.strokes = j->len_strokes;
            
                if (set.effic < min_effic) min_effic = set.effic;
                if (set.effic > max_effic) max_effic = set.effic;

                avg_effic += set.effic;
                
                wrk.sets.push_back(set);
                ++j;
            }
            rest = QTime(0,0,0).addSecs(QTime(0,0,0).secsTo(i->totalduration) - total);

            wrk.rest = rest;
            wrk.min_eff = min_effic;
            wrk.avg_eff = avg_effic / wrk.sets.size();
            wrk.max_eff = max_effic;

            workouts.push_back(wrk);
            i = j;
        }
    }


    // export
    void workoutsToSets( const std::vector<Workout>& workouts,
                         std::vector<ExerciseSet>& sets )
    {
        std::vector<Workout>::const_iterator i;
    
        for (i = workouts.begin(); i != workouts.end(); ++i)
        {
            std::vector<Set>::const_iterator j;

            int setcount = 1;
            for (j = i->sets.begin(); j!= i->sets.end(); ++j)
            {
                ExerciseSet s;

                s.user = i->user;
                s.date = i->date;
                s.time = i->time;
                s.type = i->type;
                s.pool = i->pool;
                s.unit = i->unit;
                s.totalduration = i->totalduration;
                s.cal = i->cal;
                s.lengths = i->lengths;
                s.totaldistance = i->totaldistance;

                //Recalc setids in case of deletions for compatibility with Poolmate software
                s.set = setcount++; //j->set;
                s.duration = j->duration;
                s.lens = j->lens;
                s.strk = j->strk;
                s.dist = j->dist;
                s.speed = j->speed;
                s.effic = j->effic;
                s.rate = j->rate;

                sets.push_back(s);
            }
        }
    }
} //namespace

bool DataStore::load()
{
    workouts.empty();

    sorted=false;
    changed=false;

    std::vector<ExerciseSet> input;
    if ( ReadCSV(qPrintable(filename), input))
    {
        setsToWorkouts(input, workouts);
        return true;
    }
    return false;
}

bool DataStore::save()
{
    changed=false;

    std::vector<ExerciseSet> output;
    workoutsToSets(workouts, output);
    return SaveCSV(qPrintable(filename), output );
}

//Find first exercise at date
int DataStore::findExercise(QDate dt)
{
    std::vector<Workout>::iterator i;
    int row=0;

    for (i=workouts.begin(); i != workouts.end(); ++i)
    {
        if (i->date == dt)
            return row;
        row++;
    }
    return -1;
}

int DataStore::findExercise(QDateTime dt)
{
    std::vector<Workout>::iterator i;
    int row=0;

    for (i=workouts.begin(); i != workouts.end(); ++i)
    {
        QDateTime td(i->date, i->time);

        if (td == dt)
            return row;
        row++;
    }
    return -1;
}

int sortfn(const Workout& lhs, const Workout &rhs)
{
    QDateTime l(lhs.date, lhs.time);
    QDateTime r(rhs.date, rhs.time);

    //    if (l == r) return lhs.set < rhs.set;
    return l < r;
}

void DataStore::remove(int id)
{
    workouts.erase(workouts.begin()+id);
    changed=true;
}

void DataStore::removeSet(int wid, int sid)
{
    std::vector<Set>& s = workouts[wid].sets;
    s.erase(s.begin()+sid);
    changed=true;
}

// Remove all exercises at date
void DataStore::remove( QDateTime dt )
{
    std::vector<Workout>::iterator i;
    for (i=workouts.begin(); i != workouts.end(); ++i)
    {
        if (QDateTime(i->date,i->time) == dt)
        {
            changed=true;
            workouts.erase(i);
        }
    }
}

int DataStore::add(const std::vector<ExerciseSet> &sets)
{
    sorted=false;
    changed=true;
    setsToWorkouts( sets,  workouts);
    return -1;
}

const std::vector<Workout>& DataStore::Workouts() const
{
    if (!sorted)
        std::sort(workouts.begin(), workouts.end(), sortfn);
    sorted=true;
    return workouts;
}

