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
#include "distances.h"

#include <QRadioButton>
#include <QChartView>
#include <QLineSeries>
#include <QValueAxis>
#include <QDateTimeAxis>


#define COL_DATE 0
#define COL_BEST 1
#define COL_PREDICTED 2
#define COL_REFERENCE 3
#define COL_RADIO 4


void AnalysisImpl::createTable()
{
    times.resize(distances.size());
    analysisTable->setRowCount(distances.size());

    for (size_t i = 0; i < distances.size(); ++i)
    {
        QTableWidgetItem * item = new QTableWidgetItem(distances[i].label);
        analysisTable->setVerticalHeaderItem(i, item);

        QRadioButton *button = new QRadioButton(distances[i].label, this);
        analysisTable->setCellWidget(i, COL_RADIO, button);
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
void AnalysisImpl::setBestTime(int distance, record_t & rec)
{
    rec.best = -1;

    const std::vector<Workout>& workouts = ds->Workouts();

    for (size_t i = 0; i < workouts.size(); ++i)
    {
        const Workout & w = workouts[i];

        if (w.type != "Swim" && w.type != "SwimHR")
            continue;

        const int pool = w.pool;

        if (pool <= 0)
            continue;

        for (size_t j = 0; j < w.sets.size(); ++j)
        {
            double range; // unused
            double duration;
            int actual; // unused
            if (getFastestSubset(w.sets[j], pool, distance, duration, range, actual))
            {
                if (rec.best < 0 || duration < rec.best)
                {
                    rec.best = duration;
                    rec.date = w.date;
                }
            }
        }

    }
}

AnalysisImpl::AnalysisImpl(QWidget *parent) :
    QDialog(parent), ds(NULL), distances(getDistanceFromConfig()), bestRef(-1)
{
    setupUi(this);
    createTable();
}

void AnalysisImpl::fillTable()
{
    for (size_t i = 0; i < distances.size(); ++i)
    {
        if (times[i].predicted > 0)
        {
            QTime calc = QTime(0,0,0).addSecs(times[i].predicted);
            analysisTable->setItem(i, COL_PREDICTED, createTableWidgetItem(calc.toString("hh:mm:ss")));
            const double reference = times[i].reference;

            analysisTable->setItem(i, COL_REFERENCE, createTableWidgetItem(reference));

            if (distances[i].distance == reference)
            {
                QRadioButton * radio = dynamic_cast<QRadioButton *>(analysisTable->cellWidget(i, COL_RADIO));
                radio->setChecked(true);
            }
        }
        else
        {
            analysisTable->setItem(i, COL_PREDICTED, createTableWidgetItem(""));
            analysisTable->setItem(i, COL_REFERENCE, createTableWidgetItem(""));
        }
    }

    if (bestRef >= 0)
    {
        for (int i = 0; i < analysisTable->columnCount(); ++i)
        {
            QTableWidgetItem * item = analysisTable->item(bestRef, i);
            if (item)
            {
                item->setBackground(QBrush(Qt::green));
            }
        }
    }

    analysisTable->resizeColumnsToContents();
}

int AnalysisImpl::precalculate(const double minimum)
{
    std::vector<double> normalised(distances.size(), std::numeric_limits<double>::infinity());
    // Populate best times
    for (size_t i = 0; i < distances.size(); ++i)
    {
        record_t & rec = times[i];

        setBestTime(distances[i].distance, rec);
        QWidget * radio = analysisTable->cellWidget(i, 4);

        if (rec.best > 0)
        {
            if (distances[i].distance >= minimum)
            {
                // we skip predictions based on very short distances as less accurate
                normalised[i] = rec.best / std::pow(distances[i].distance, peterRiegelExponent);
            }

            const QTime calc = QTime(0, 0, 0).addSecs(rec.best);
            analysisTable->setItem(i, COL_BEST, createTableWidgetItem(calc.toString("hh:mm:ss")));
            analysisTable->setItem(i, COL_DATE, createTableWidgetItem(rec.date));
            radio->setEnabled(true);
        }
        else
        {
            analysisTable->setItem(i, COL_BEST, createTableWidgetItem(""));
            analysisTable->setItem(i, COL_DATE, createTableWidgetItem(""));
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
    for (size_t i = 0; i < distances.size(); ++i)
    {
        if (ref >=0)
        {
            // Use Peter Riegel's formula: t2 = t1 * (d2 / d1) ^ 1.06.
            times[i].predicted = times[ref].best * pow(distances[i].distance / distances[ref].distance, peterRiegelExponent);
            times[i].reference = distances[ref].distance;
        }
        else
        {
            times[i].predicted = 0.0;
            times[i].reference = 0.0;
        }
    }

    fillTable();
    fillChart();
}

void AnalysisImpl::fillChart()
{
    QtCharts::QLineSeries *series[2] = {new QtCharts::QLineSeries(), new QtCharts::QLineSeries()};
    series[0]->setName("best");
    series[1]->setName("predicted");

    // Y axis is speed in ms per 100m (see QDateTimeAxis)
    const double multiplier = 100 * 1000;

    for (size_t i = 0; i < distances.size(); ++i)
    {
        if (times[i].best > 0)
        {
            series[0]->append(distances[i].distance, times[i].best / distances[i].distance * multiplier);
        }
        if (times[i].predicted > 0)
        {
            series[1]->append(distances[i].distance, times[i].predicted / distances[i].distance * multiplier);
        }
    }

    QtCharts::QChart *chart = new QtCharts::QChart();

    QtCharts::QDateTimeAxis *axisY = new QtCharts::QDateTimeAxis;
    axisY->setTitleText("speed");
    axisY->setFormat("mm:ss");
    axisY->setTickCount(8);
    chart->addAxis(axisY, Qt::AlignLeft);

    QtCharts::QValueAxis *axisX = new QtCharts::QValueAxis;
    axisX->setTitleText("distance");
    axisX->setLabelFormat("%.0fm");
    axisX->setTickInterval(200);
    axisX->setTickAnchor(200);
    axisX->setTickType(QtCharts::QValueAxis::TicksDynamic);
    chart->addAxis(axisX, Qt::AlignBottom);

    for (QtCharts::QLineSeries *serie : series)
    {
        chart->addSeries(serie);
        serie->attachAxis(axisX);
        serie->attachAxis(axisY);
    }

    setChartOnView(chartView, chart);
}

void AnalysisImpl::setDataStore(const DataStore *_ds)
{
    ds = _ds;
    // times below 100m are less reliable to determine best fit
    bestRef = precalculate(100.0);
    fillPredicted(bestRef);
}

void AnalysisImpl::on_allBest_clicked()
{
    BestTimesImpl win(this);

    win.setDataStore(ds);
    win.exec();
}
