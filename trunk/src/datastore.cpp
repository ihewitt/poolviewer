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
bool SaveCSV( const std::string & name, std::vector<ExerciseSet>& exercises, bool extra )
{
    QFile file(name.c_str());
    if (file.open(QIODevice::WriteOnly|QIODevice::Text))
    {
        QTextStream out(&file);

        out << "User,LogDate,LogTime,LogType,Pool,Units,TotalDuration,Calories,TotalLengths,TotalDistance,Nset,Duration,Strokes,Distance,Speed,Efficiency,StrokeRate,HRmin,HRmax,HRavg,HRbegin,HRend,Version\n";
        
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
                    << i->dist << ","
                    << i->speed << ","
                    << i->effic << ","
                    << i->rate << ",";
            }
            else
            {
                out << i->type << ","
                    << ",," 
                    << i->totalduration.toString("hh:mm:ss") << ","
                    << ",,,"
                    << i->set << ","
                    << i->duration.toString("hh:mm:ss") << ","
                    << ",,,,,";
            }
            out << ",,,,,210\n";
        }
    }
return true;
}

//
bool ReadCSV( const std::string & name, std::vector<ExerciseSet>& dst, bool extra )
{
// Simplistic csv format reading
    QFile file(name.c_str());
    if (file.open(QIODevice::ReadOnly))
    {
        file.readLine(); //skip header

        while (!file.atEnd())
        {
            QString line = file.readLine();
            QStringList strings = line.split(",");

            ExerciseSet e;

            e.user = strings.value(0).toInt();
            e.date = QDate::fromString( strings.value(1), QString("dd/MM/yyyy"));
            e.time = QTime::fromString( strings.value(2));

            e.type = strings.value(3);
            e.pool = strings.value(4).toInt();
            e.unit = strings.value(5);
            e.totalduration = QTime::fromString(strings.value(6));
            e.cal = strings.value(7).toInt();
            e.lengths = strings.value(8).toInt();
            e.totaldistance = strings.value(9).toInt();
            e.set = strings.value(10).toInt();
            e.duration = QTime::fromString( strings.value(11));
            e.strk = strings.value(12).toInt();
            e.dist  = strings.value(13).toInt();
            e.speed  = strings.value(14).toInt();
            e.effic = strings.value(15).toInt();
            e.rate  = strings.value(16).toInt();

            dst.push_back(e);
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

                s.set = j->set;
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
    if ( ReadCSV(qPrintable(filename), input, true))
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
    return SaveCSV(qPrintable(filename), output, true);
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

//    printf("%s\n", qPrintable(dt.toString()));

    for (i=workouts.begin(); i != workouts.end(); ++i)
    {
		QDateTime td(i->date, i->time);

//                printf("%s\n", qPrintable(td.toString()));

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

