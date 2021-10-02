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
#include "besttimesimpl.h"
#include "utilities.h"
#include "datastore.h"

#include <QRadioButton>

namespace
{
    const double distances[] = {25, 50, 100, 200, 400, 800, 1000, 1500, 1931, 2000, 3862, 5000};
    const char * labels[] = {"25m", "50m", "100m", "200m", "400m", "800m", "1000m", "1500m", "1931m (1.2ml)", "2000m", "3862m (2.4ml)", "5000m"};
    const size_t numberOfRows = sizeof(distances) / sizeof(distances[0]);
}


void AnalysisImpl::createTable()
{
    times.resize(numberOfRows, std::vector<double>(3));
    analysisTable->setRowCount(numberOfRows);

    for (size_t i = 0; i < numberOfRows; ++i)
    {
        QTableWidgetItem * item = new QTableWidgetItem(labels[i]);
        analysisTable->setVerticalHeaderItem(i, item);

        QRadioButton *button = new QRadioButton(labels[i], this);
        analysisTable->setCellWidget(i, 3, button);
        const auto handler = [this, i] (const bool x) {
            if (x)
            {
                this->fillPredicted(i);
            }
        };
        connect(button, &QRadioButton::toggled, handler);
    }
}

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
            //const int distance = w.pool * numberOfLanes;
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
    createTable();
}

void AnalysisImpl::fillTable()
{
    for (size_t i = 0; i < numberOfRows; ++i)
    {
        if (times[i][1] > 0)
        {
            QTime calc = QTime(0,0,0).addSecs(times[i][1]);
            analysisTable->setItem(i, 1, createTableWidgetItem(calc.toString("hh:mm:ss")));
            const double reference = times[i][2];

            analysisTable->setItem(i, 2, createTableWidgetItem(reference));

            if (distances[i] == reference)
            {
                QRadioButton * radio = dynamic_cast<QRadioButton *>(analysisTable->cellWidget(i, 3));
                radio->setChecked(true);
            }
        }
        else
        {
            analysisTable->setItem(i, 1, createTableWidgetItem("none"));
            analysisTable->setItem(i, 2, createTableWidgetItem("none"));
        }
    }
}

int AnalysisImpl::precalculate(const double minimum)
{
    std::vector<double> normalised(numberOfRows, std::numeric_limits<double>::infinity());
    // Populate best times
    for (size_t i = 0; i < numberOfRows; ++i)
    {
        const double best = getBestTime(distances[i]);
        times[i][0] = best;
        QWidget * radio = analysisTable->cellWidget(i, 3);

        if (best > 0)
        {
            if (distances[i] >= minimum)
            {
                // we skip predictions based on very short distances as less accurate
                normalised[i] = best / std::pow(distances[i], peterRiegelExponent);
            }

            const QTime calc = QTime(0, 0, 0).addSecs(best);
            analysisTable->setItem(i, 0, createTableWidgetItem(calc.toString("hh:mm:ss")));
            radio->setEnabled(true);
        }
        else
        {
            analysisTable->setItem(i, 0, createTableWidgetItem("none"));
            radio->setEnabled(false);
        }
    }

    // and default reference
    const std::vector<double>::iterator min_it = std::min_element(normalised.begin(), normalised.end());
    if (isfinite(*min_it))
    {
        const int ref = std::distance(normalised.begin(), min_it);
        return ref;
    }
    else
    {
        return -1;
    }
}

void AnalysisImpl::fillPredicted(const int ref)
{
    // For each time / distance predict best time
    for (size_t i = 0; i < numberOfRows; ++i)
    {
        if (ref >=0 && times[i][0] > 0)
        {
            // Use Peter Riegel's formula: t2 = t1 * (d2 / d1)^1.06.
            times[i][1] = times[ref][0] * pow(distances[i] / distances[ref], peterRiegelExponent);
            times[i][2] = distances[ref];
        }
        else
        {
            times[i][1] = 0.0;
            times[i][2] = 0.0;
        }
    }

    fillTable();
}

void AnalysisImpl::setDataStore(const DataStore *_ds)
{
    ds = _ds;
    // times below 100m are less reliable
    const int ref = precalculate(10000.0);
    fillPredicted(ref);
}

void AnalysisImpl::on_allBest_clicked()
{
    BestTimesImpl win(this);

    win.setDataStore(ds);
    win.exec();
}
