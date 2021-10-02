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

#ifndef BESTTIMESIMPL_H
#define BESTTIMESIMPL_H

#include "ui_besttimesimpl.h"

class DataStore;

class BestTimesImpl : public QDialog, private Ui::besttimesimpl
{
    Q_OBJECT

public:
    explicit BestTimesImpl(QWidget *parent = 0);

    void setDataStore(const DataStore *_ds);

private slots:
    void on_calculateButton_clicked();

    void on_progressionBox_clicked();

private:
    const DataStore *ds;

    void fillChart(const std::vector<QTime> & allTimes);
};

#endif // BESTTIMESIMPL_H
