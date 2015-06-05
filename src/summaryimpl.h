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

#ifndef SUMMARYIMPL_H
#define SUMMARYIMPL_H
//
#include <QDialog>
#include "ui_summary.h"

#include "datastore.h"

//
class DataStore;
class SummaryImpl : public QDialog, public Ui::Summary
{
    enum Scale
    {
        WORKOUTS,
        WEEKLY,
        MONTHLY,
        YEARBYWEEK,
        YEARBYMONTH
    };

Q_OBJECT
public:
    SummaryImpl( QWidget * parent = 0, Qt::WindowFlags f = 0 );

    void setData();
    void setData( const std::vector<Workout>& workouts);
    void setData( const Workout& workout );

    void fillWorkouts( const std::vector<Workout>& workouts );
    void fillSets( const std::vector<Set>& sets );
    void fillLengths( const Set& set);

    void colorRow(int r, QColor c); 

    void setDataStore(DataStore *_ds) { ds = _ds;}

 protected:
    void keyPressEvent(QKeyEvent *event);
    void closeEvent(QCloseEvent *event);
    
 private slots:
    virtual void selectedDate(QDate);
    virtual void workoutSelected();
    virtual void setSelected();
    virtual void syncButton();
    virtual void configButton();
    virtual void editButton();
    virtual void deleteClick();
    virtual void scaleChanged(int);
    virtual void printButton();
    virtual void on_check_clicked();

private:
    DataStore *ds;
    Scale scale;
    bool setSel; //this is a bit clunky
};
#endif




