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

#ifndef ANALYSISIMPL_H
#define ANALYSISIMPL_H

#include "ui_analysis.h"
#include "distances.h"

#include <QDate>

class DataStore;

class AnalysisImpl :  public QDialog, private Ui::Analysis
{
    Q_OBJECT

public:
    explicit AnalysisImpl(QWidget *parent = 0);
    void setDataStore(const DataStore *_ds);


private slots:
    void on_allBest_clicked();

private:
    struct record_t
    {
        double best;
        double predicted;
        double reference;
        QDate date;
    };

    void setBestTime(int distance, record_t & rec);
    void createTable();
    void fillTable();
    void fillChart();
    int precalculate(const double minimum);
    void fillPredicted(const int ref);

    const DataStore *ds;
    const std::vector<Distance> distances;

    std::vector<record_t> times;
    int bestRef;

    static constexpr double peterRiegelExponent = 1.06;
};

#endif // ANALYSISIMPL_H
