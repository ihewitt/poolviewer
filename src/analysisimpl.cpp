/*
 * This file is part of PoolViewer
 * Copyright (c) 2015 Ivor Hewitt
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
#include <math.h>

#include "analysisimpl.h"
#include "utilities.h"
#include "datastore.h"


const double distance[]={25,100,400,750,1500,1931,3862,5000};

// Add 'when' column
double AnalysisImpl::getBestTime(int distance)
{
    double best = -1;

    const std::vector<Workout>& workouts = ds->Workouts();

    for (size_t i = 0; i < workouts.size(); ++i)
    {
        const Workout & w = workouts[i];

        if (w.type != "Swim" && w.type != "SwimHR")
            continue;

        const int pool = w.pool;

        if (pool <= 0)
            continue;

        // this is the number of lanes to get above or equal the desired distance
        const int numberOfLanes = (distance + pool - 1) / pool; // round up

        for (size_t j = 0; j < w.sets.size(); ++j)
        {
            const Set & set = w.sets[j];
            if (set.lens < numberOfLanes)
                // not enough in this set
                continue;

            //
            const int distance = w.pool * numberOfLanes;
            double duration;

            if ((int)set.times.size() != set.lens)
            {
                // Non poolmate live, try to use set duration.
                // Since we're taking distances >= numberOfLanes which should be slower, assume we can
                // just divide to get the closes time for shorter distance.
                duration = (QTime(0,0).secsTo(set.duration) * numberOfLanes / set.lens);
            }
            else
            {
                // Locate fastest window
                double min = 0.0;
                for (int j = 0; j <= (int)set.times.size()-numberOfLanes; ++j)
                {
                    double cur = 0.0;
                    for (int i = j; i < j + numberOfLanes; ++i)
                    {
                        cur += set.times[i];
                    }
                    if (cur < min || min == 0.0)
                        min = cur;
                }
                duration = min;
            }

            if (best < 0 || duration < best)
                best = duration;
        }

    }
    return best;
}

AnalysisImpl::AnalysisImpl(QWidget *parent) :
    QDialog(parent),ds(NULL)
{
    setupUi(this);
}

void AnalysisImpl::fillTable()
{
    int i;
    for (i=0;i<8;++i)
    {
        if (times[i][0]>0)
        {
            QTime best = QTime(0,0,0).addSecs(times[i][0]);
            analysisTable->setItem(i, 0, createTableWidgetItem(best.toString("hh:mm:ss")));
        }
        else
        {
            analysisTable->setItem(i, 0, createTableWidgetItem("none"));
        }

        if (times[i][1]>0)
        {
            QTime calc = QTime(0,0,0).addSecs(times[i][1]);
            analysisTable->setItem(i, 1, createTableWidgetItem(calc.toString("hh:mm:ss")));
        }
        else
        {
            analysisTable->setItem(i, 1, createTableWidgetItem("none"));
        }
        analysisTable->setItem(i,2, createTableWidgetItem(times[i][2]));
    }
}

void AnalysisImpl::showEvent(QShowEvent *ev)
{
    QDialog::showEvent(ev);

    //Populate times
    int i;
    for (i=0;i<8;++i)
    {
        double best = getBestTime(distance[i]);
        times[i][0] = best;
        times[i][1] = -1;
    }

    // For each time/distance predict best time
    for (i=1; i<8; ++i) //start at 100m for intial calc
    {
        if (times[i][0] > 0)
        {
            int j;
            for (j=0;j<8;++j)
            {
                if (i == j && times[j][1] > times[i][0])
                {
                    times[j][1] = times[i][0];
                    times[j][2] = distance[i];
                }
                else
                {
                    //Use Peter Riegel's formula: t2 = t1 * (d2 / d1)^1.06.
                    double calc = times[i][0] * pow(distance[j]/distance[i],1.06);

                    if (times[j][1] < 0 || times[j][1] > calc)
                    {
                        times[j][1] = calc;
                        times[j][2] = distance[i];
                    }
                }
            }
        }
    }

    fillTable();
}

void AnalysisImpl::setDataStore(const DataStore *_ds)
{
    ds = _ds;
}

void AnalysisImpl::on_calcButton_clicked()
{
    int row = analysisTable->currentRow();
    double dist = distance[row];

    double best = times[row][0];
    int i;
    for (i=0; i<8; ++i)
    {
        //Use Peter Riegel's formula: t2 = t1 * (d2 / d1)^1.06.
        double calc = best * pow(distance[i]/distance[row],1.06);

        times[i][1]=calc;
        times[i][2]=dist;
    }

    fillTable();
}
