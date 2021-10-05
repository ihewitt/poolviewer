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

#include "besttimesimpl.h"
#include "datastore.h"
#include "utilities.h"

#include <QSettings>
#include <QChartView>
#include <QLineSeries>
#include <QScatterSeries>
#include <QDateTimeAxis>
#include <QtMath>

namespace
{
    void addSet(const int targetDistance, const Set & set, const Workout & workout, double & bestSpeed,
                QTableWidget * table, std::vector<BestTimesImpl::record_t> &allTimes)
    {
        double range; // time range per length: slowest - fastest
        double bestTime; // adjusted for the distance requested!
        int distance; // distance used for time (might be longer)
        if (!getFastestSubset(set, workout.pool, targetDistance, bestTime, range, distance))
        {
            return;  // not enought lengths
        }

        const bool speedAsMinuteAndSeconds = QSettings("Swim", "Poolmate").value("speed").toBool();
        const QTime duration = QTime(0, 0).addMSecs(bestTime * 1000.0);

        allTimes.push_back({duration.msecsSinceStartOfDay(), QDateTime(workout.date, workout.time)});

        const double speed = bestTime / targetDistance * 100.0;
        const double total = workout.pool * set.lens;  // total distance in set

        const int row = table->rowCount();
        table->setRowCount(row + 1);

        QTableWidgetItem * dateItem = createTableWidgetItem(QVariant(workout.date));
        table->setItem(row, 0, dateItem);

        QTableWidgetItem * timeItem = createTableWidgetItem(QVariant(workout.time));
        table->setItem(row, 1, timeItem);

        QTableWidgetItem * poolItem = createTableWidgetItem(QVariant(workout.pool));
        table->setItem(row, 2, poolItem);

        QTableWidgetItem * distanceItem = createTableWidgetItem(QVariant(distance));
        table->setItem(row, 3, distanceItem);

        QTableWidgetItem * durationItem = createTableWidgetItem(QVariant(duration.toString()));
        table->setItem(row, 4, durationItem);

        QTableWidgetItem * speedItem = createTableWidgetItem(QVariant(formatSpeed(speed, speedAsMinuteAndSeconds)));
        table->setItem(row, 5, speedItem);

        QTableWidgetItem * totalItem = createTableWidgetItem(QVariant(total));
        table->setItem(row, 6, totalItem);

        QTableWidgetItem * rangeItem = createTableWidgetItem(QVariant(range));
        table->setItem(row, 7, rangeItem);

        // Store in column 0 whether this has been a new best time
        // we can use speed or duration as they are uniform now
        const bool record = speed <= bestSpeed;
        if (record)
        {
            bestSpeed = speed;
        }

        dateItem->setData(NEW_BEST_TIME, QVariant(record));
    }
}

BestTimesImpl::BestTimesImpl(QWidget *parent) :
    QDialog(parent), ds(NULL)
{
    setupUi(this);

    // not done in on_calculateButton_clicked() not to override user choice

    // sort by duration as speed loses precision
    timesTable->sortItems(4);
    chartView->setRubberBand(QtCharts::QChartView::HorizontalRubberBand);
}

void BestTimesImpl::setDataStore(const DataStore *_ds)
{
    ds = _ds;
}

void BestTimesImpl::on_calculateButton_clicked()
{
    enum period
    {
        ALL = 0,
        THISMONTH,
        THISYEAR,
        LASTMONTH,
        LASTYEAR
    };

    timesTable->clearContents();
    timesTable->setRowCount(0);

    // otherwise the rows move around while they are added
    // and we set the item in the wrong place
    timesTable->setSortingEnabled(false);

    int id = periodBox->currentIndex();

    const QString distStr = distanceBox->currentText();
    bool ok = false;
    const uint distance = distStr.toUInt(&ok);
    if (!ok || distance == 0)
    {
        return;
    }

    double bestSpeed = std::numeric_limits<double>::max();

    const std::vector<Workout>& workouts = ds->Workouts();

    std::vector<record_t> allTimes;

    for (size_t i = 0; i < workouts.size(); ++i)
    {
        const Workout & w = workouts[i];

        switch(id)
        {
        case ALL:
            break;

        case THISMONTH:
        {
            QDate now = QDate::currentDate();
            if (w.date.month() != now.month() ||
                    w.date.year() != now.year())
                continue;
            break;
        }

        case THISYEAR:
        {
            QDate now = QDate::currentDate();
            if (w.date.year() != now.year())
                continue;
            break;
        }

        case LASTMONTH:
        {
            QDate then = QDate::currentDate().addMonths(-1);
            if (w.date.month() != then.month() ||
                    w.date.year() != then.year())
                continue;
            break;
        }

        case LASTYEAR:
        {
            QDate then = QDate::currentDate().addYears(-1);
            if (w.date.year() != then.year())
                continue;
            break;
        }
        }

        if (w.type != "Swim" && w.type != "SwimHR")
        {
            continue;
        }

        const int pool = w.pool;

        if (pool <= 0)
        {
            continue;
        }

        for (size_t j = 0; j < w.sets.size(); ++j)
        {
            addSet(distance, w.sets[j], w, bestSpeed, timesTable, allTimes);
        }
    }

    fillDistributionChart(allTimes);
    fillScatterChart(allTimes);

    // ensure the state of the "record progression" is correct
    on_progressionBox_clicked();
    timesTable->resizeColumnsToContents();
    timesTable->setSortingEnabled(true);
}

void BestTimesImpl::fillDistributionChart(const std::vector<record_t> & allTimes)
{
    QtCharts::QChart *chart = new QtCharts::QChart();
    if (allTimes.size() > 1)
    {
        const auto compareDuration = [](const record_t & lhs, const record_t & rhs) {return lhs.duration < rhs.duration;};
        const auto compareDate = [](const record_t & lhs, const record_t & rhs) {return lhs.date < rhs.date;};

        std::vector<record_t> sorted(allTimes);
        std::sort(sorted.begin(), sorted.end(), compareDuration);

        const int minTime = sorted.front().duration;
        const int maxTime = sorted.back().duration;
        const int span = maxTime - minTime;

        QtCharts::QLineSeries *series = new QtCharts::QLineSeries();
        series->setName("distribution");
        QtCharts::QLineSeries *medianSeries = new QtCharts::QLineSeries();
        medianSeries->setName("median");
        QtCharts::QScatterSeries *dateSeries = new QtCharts::QScatterSeries();
        dateSeries->setName("date");

        // heuristic to get a decent number of bins
        const size_t bins = qSqrt(sorted.size()) * 2.5;
        std::vector<record_t>::iterator previous = sorted.begin();
        int maximum = 0; // to fit the median vertically

        for (size_t i = 0; i < bins; ++i)
        {
            const int currentTime = minTime + span * i / (bins - 1);
            const record_t value = {currentTime, QDateTime()};

            const std::vector<record_t>::iterator it = std::upper_bound(sorted.begin(), sorted.end(), value, compareDuration);
            const int diff = std::distance(previous, it);
            const QDateTime mostRecent = std::max_element(previous, it, compareDate)->date;

            series->append(currentTime, diff);
            dateSeries->append(currentTime, mostRecent.toMSecsSinceEpoch());

            maximum = std::max(maximum, diff);
            previous = it;
        }

        const int median = sorted[sorted.size() / 2].duration;
        medianSeries->append(median, 0);
        medianSeries->append(median, maximum);

        QtCharts::QDateTimeAxis *axisX = new QtCharts::QDateTimeAxis;
        axisX->setTitleText("time");
        axisX->setFormat("mm:ss");
        axisX->setTickCount(13);
        chart->addAxis(axisX, Qt::AlignBottom);

        QtCharts::QDateTimeAxis *axisY = new QtCharts::QDateTimeAxis;
        axisY->setFormat("MMM yy");
        chart->addAxis(axisY, Qt::AlignRight);

        chart->addSeries(series);
        series->attachAxis(axisX);
        // no y axis as values are meaningless

        chart->addSeries(medianSeries);
        medianSeries->attachAxis(axisX);
        // no y axis as values are meaningless

        chart->addSeries(dateSeries);
        dateSeries->attachAxis(axisX);
        dateSeries->attachAxis(axisY);

        // make y axis a bit wider so we can fit the xy markers
        enlargeAxis(axisY, 0.025);
    }

    setChartOnView(chartView, chart);
}

void BestTimesImpl::fillScatterChart(const std::vector<record_t> & allTimes)
{
    QtCharts::QChart *chart = new QtCharts::QChart();
    if (!allTimes.empty())  // even if with 1 it does not show anything
    {
        QtCharts::QScatterSeries *series = new QtCharts::QScatterSeries();

        for (const auto & run : allTimes)
        {
            series->append(run.date.toMSecsSinceEpoch(), run.duration);
        }

        QtCharts::QDateTimeAxis *axisX = new QtCharts::QDateTimeAxis;
        axisX->setFormat("MMM yy");
        chart->addAxis(axisX, Qt::AlignBottom);

        QtCharts::QDateTimeAxis *axisY = new QtCharts::QDateTimeAxis;
        axisY->setFormat("mm:ss");
        chart->addAxis(axisY, Qt::AlignRight);

        chart->addSeries(series);
        series->attachAxis(axisX);
        series->attachAxis(axisY);

        // make axes a bit wider so we can fit the xy markers
        enlargeAxis(axisX, 0.025);
        enlargeAxis(axisY, 0.025);

        chart->legend()->hide();
    }

    setChartOnView(scatterView, chart);
}

void BestTimesImpl::on_progressionBox_clicked()
{
    const bool checked = progressionBox->checkState() == Qt::Checked;
    const size_t numberOfTimes = timesTable->rowCount();
    if (checked)
    {
        for (size_t i = 0; i < numberOfTimes; ++i)
        {
            const QTableWidgetItem * record_item = timesTable->item(i, 0);
            const bool record = record_item->data(NEW_BEST_TIME).toBool();
            timesTable->setRowHidden(i, !record);
        }
    }
    else
    {
        for (size_t i = 0; i < numberOfTimes; ++i)
        {
            timesTable->setRowHidden(i, false);
        }
    }
}
