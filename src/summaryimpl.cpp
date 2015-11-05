/*
 * This file is part of PoolViewer
 * Copyright (c) 2011-2015 Ivor Hewitt
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

#include <stdio.h>

#include <QSettings>
#include <QFileDialog>
#include <QTextCharFormat>
#include <QCloseEvent>
#include <QMessageBox>
#include <QPainter>
#include <QPrinter>
#include <QPrintDialog>

#include "summaryimpl.h"
#include "uploadimpl.h"
#include "syncimpl.h"
#include "datastore.h"
#include "configimpl.h"
#include "analysisimpl.h"
#include "edit.h"
#include "utilities.h"


SummaryImpl::SummaryImpl( QWidget * parent, Qt::WindowFlags f)
    : QDialog(parent, f)
{
    setupUi(this);
    setWindowFlags(Qt::WindowMinimizeButtonHint|Qt::WindowMaximizeButtonHint);

    // Graph control settings
    efficCheck->setChecked(true);
    strokeCheck->setChecked(true);
    rateCheck->setChecked(true);
    speedCheck->setChecked(true);
    graphWidget->setGraphs(true,true,true,true);

    lengthWidget->setGraphs(true,true,true,true);
    scale = WORKOUTS;
    tabs->setTabEnabled(0,false);
    tabs->setCurrentIndex(1);

    //    setEscapeButton(pushButton);
}

void SummaryImpl::scaleChanged(int sc)
{
    scale = (Scale)sc;

    switch (scale)
    {
    case WORKOUTS:
        tabs->setTabEnabled(0,false);
        tabs->setTabEnabled(1,true);
        tabs->setTabEnabled(2,true);
        tabs->setCurrentIndex(1);
        break;
    default:
        tabs->setTabEnabled(0,true);
        tabs->setTabEnabled(1,true);
        tabs->setTabEnabled(2,false);
        tabs->setCurrentIndex(0);
        break;
    }

    setData(ds->Workouts());
    volumeWidget->update();
    graphWidget->update();
    lengthWidget->update();
}

/*
 * populate graph control based on grouping settings
 * TODO break this up.
 */
void SummaryImpl::setData( const std::vector<Workout>& workouts)
{
    //current date/selection
    QDate day = calendarWidget->selectedDate();

    QDate start, end;

    switch (scale)
    {
    case WORKOUTS:
    {
        start = day;
        end = day;
    }
        break;
    case WEEKLY:
    {
        start = day.addDays(-day.dayOfWeek());
        end = day.addDays(7-day.dayOfWeek());
    }
        break;
    case MONTHLY:
    {
        start = day.addDays(-day.day());
        end = day.addDays(day.daysInMonth()-day.day());
    }
        break;
    case YEARBYWEEK:
    {
        int yr = day.year();
        start = QDate(yr,1,1);
        end = QDate(yr,12,31);

    }
        break;
    case YEARBYMONTH:
    {
        int yr = day.year();
        start = QDate(yr,1,1);
        end = QDate(yr,12,31);

    }
        break;
    };

    volumeWidget->clear();
    volumeWidget->style = GraphWidget::Bar;
    volumeWidget->series.resize(4);
    volumeWidget->series[0].label = "Cal";
    volumeWidget->series[1].label = "Dist";
    volumeWidget->series[2].label = "";
    volumeWidget->series[3].label = "Dur/Rest";

    //    if (scale != WORKOUTS)
    {
        graphWidget->clear();
        graphWidget->style = GraphWidget::Line;
        graphWidget->series.resize(4);
        graphWidget->series[0].label = "Eff";
        graphWidget->series[1].label = "Spd";
        graphWidget->series[2].label = "Strk";
        graphWidget->series[3].label = "Rte";
    }
    {
        lengthWidget->clear();
        lengthWidget->style = GraphWidget::Line;
        lengthWidget->series.resize(4);
        lengthWidget->series[0].label = "Eff";
        lengthWidget->series[1].label = "Spd";
        lengthWidget->series[2].label = "Strk";
        lengthWidget->series[3].label = "Rte";
    }

    if (scale == WORKOUTS)
    {
        int row = 0;
        if (ds->Workouts().size())
        {
            const Workout& workout = ds->Workouts()[workoutGrid->currentRow()];

            std::vector<Set>::const_iterator i;
            for (i = workout.sets.begin(); i != workout.sets.end(); ++i)
            {
                graphWidget->xaxis.push_back( QString::number(++row) );
                graphWidget->series[0].integers.push_back(i->effic);
                graphWidget->series[1].integers.push_back(i->speed);
                graphWidget->series[2].doubles.push_back( i->strk ? (double)workout.pool/i->strk : 0.0 );
                graphWidget->series[3].integers.push_back(i->rate);
            }

            if (workout.sets[0].times.size())
            {
                int n=0;
                std::vector<Set>::const_iterator i;
                for (i = workout.sets.begin(); i != workout.sets.end(); ++i)
                {
                    int j;
                    for ( j=0; j < i->lens; ++j )
                    {
                        double rate = 60 * i->strokes[j]/i->times[j];
                        int effic = ((25 * i->times[j]) + (25*i->strokes[j]))/workout.pool;

                        lengthWidget->xaxis.push_back(QString::number(++n));

                        lengthWidget->series[0].integers.push_back(effic);
                        lengthWidget->series[1].integers.push_back(100*i->times[j]/workout.pool);
                        lengthWidget->series[2].doubles.push_back((double)workout.pool/i->strokes[j]);
                        lengthWidget->series[3].integers.push_back(rate);
                    }
                    lengthWidget->xaxis.push_back(QString());
                    lengthWidget->series[0].integers.push_back(-1);
                    lengthWidget->series[1].integers.push_back(-1);
                    lengthWidget->series[2].doubles.push_back(-1);
                    lengthWidget->series[3].integers.push_back(-1);
                }
            }
        }
    }
    else
    {
        std::vector<Workout>::const_iterator i;
        for ( i = workouts.begin(); i != workouts.end(); ++i)
        {
            if ( (i->type == "Swim"|| i->type=="SwimHR") &&
                 i->date >= start &&
                 i->date <= end)
            {
                QString axLabel;
                if (scale == YEARBYWEEK)
                    axLabel=QString("%1")
                            .arg(i->date.weekNumber());
                else if (scale == YEARBYMONTH)
                    axLabel=i->date.toString("MMM");
                else
                    axLabel=i->date.toString("dd/MM");
                volumeWidget->xaxis.push_back(axLabel);

                volumeWidget->series[0].integers.push_back(i->cal);
                volumeWidget->series[1].integers.push_back(i->totaldistance);
                volumeWidget->series[2].seconds.push_back(QTime(0,0,0).secsTo(i->rest));
                volumeWidget->series[3].seconds.push_back(QTime(0,0,0).secsTo(i->totalduration));

                std::vector<Set>::const_iterator j;
                for (j = i->sets.begin(); j != i->sets.end(); ++j)
                {
                    graphWidget->xaxis.push_back( axLabel );
                    graphWidget->series[0].integers.push_back(j->effic);
                    graphWidget->series[1].integers.push_back(j->speed);
                    graphWidget->series[2].doubles.push_back( j->strk ? (double)i->pool/j->strk : 0.0 );
                    graphWidget->series[3].integers.push_back(j->rate);
                }
            }
        }
    }
}


void SummaryImpl::fillWorkouts( const std::vector<Workout>& workouts)
{
    workoutGrid->clearContents();

    std::vector<Workout>::const_iterator i;

    workoutGrid->setRowCount(workouts.size());

    std::map<QDate, CalendarWidget::Totals> day_totals;
    std::map<QDate, int> day_nums;

    int row=0;
    for ( i = workouts.begin(); i != workouts.end(); ++i)
    {
        if (i->max_eff > day_totals[i->date].max_eff)
        {
            day_totals[i->date].max_eff = i->max_eff;
        }

        day_totals[i->date].dist += i->totaldistance;
        day_totals[i->date].avg_eff += i->avg_eff;
        day_nums[i->date] = day_nums[i->date]+1;;

        if (i->min_eff < day_totals[i->date].min_eff || day_totals[i->date].min_eff == 0)
        {
            day_totals[i->date].min_eff = i->min_eff;
        }

        int col=0;
        QTableWidgetItem *item;

        item = createTableWidgetItem(QVariant(i->date));
        workoutGrid->setItem( row, col++, item );

        item = createTableWidgetItem(QVariant(i->time));
        workoutGrid->setItem( row, col++, item );

        if (i->type == "Swim" || i->type=="SwimHR")
        {
            item = createTableWidgetItem(QVariant(i->pool));
            workoutGrid->setItem( row, col++, item );

            item = createTableWidgetItem(QVariant(i->totalduration.toString()));
            workoutGrid->setItem( row, col++, item );

            item = createTableWidgetItem(QVariant(i->lengths));
            workoutGrid->setItem( row, col++, item );

            item = createTableWidgetItem(QVariant(i->totaldistance));
            workoutGrid->setItem( row, col++, item );

            item = createTableWidgetItem(QVariant(i->cal));
            workoutGrid->setItem( row, col++, item );

            item = createTableWidgetItem(QVariant(i->rest.toString()));
            workoutGrid->setItem( row, col++, item );
        }
        row++;
    }

    std::map<QDate, int>::iterator j;
    for (j=day_nums.begin(); j!=day_nums.end();++j)
    {
        /*        printf("%s - %d %d %d %d\n",
               qPrintable(j->first.toString("dd/MM")),
               day_totals[j->first].min_eff,
               day_totals[j->first].avg_eff,
               day_totals[j->first].max_eff,
               j->second);*/

        if (j->second > 1)
        {
            day_totals[j->first].avg_eff /= j->second;
            day_totals[j->first].dist /= j->second;
        }
    }

    workoutGrid->selectRow(workoutGrid->rowCount()-1);
    workoutGrid->resizeColumnsToContents();

    //populate graph
    setData(ds->Workouts());
    calendarWidget->setData(day_totals);
}

void SummaryImpl::fillLengths( const Workout& wrk)
{
    lengthGrid->clearContents();
    lengthGrid->setRowCount(wrk.lengths);

    const std::vector<Set>& sets = wrk.sets;

    int set=0;
    int row=0;
    std::vector<Set>::const_iterator it;
    for (it=sets.begin(); it != sets.end(); ++it)
    {
        int i;
        for (i = 0; i < it->lens; ++i)
        {
            uint col=0;
            QTableWidgetItem *item;

            item = createTableWidgetItem(QVariant(1 + set));
            lengthGrid->setItem( row, col++, item );

            item = createTableWidgetItem(QVariant(1 + i));
            lengthGrid->setItem( row, col++, item );

            item = createTableWidgetItem(QVariant(QString::number(it->times[i],'f',3)));
            lengthGrid->setItem( row, col++, item );

            item = createTableWidgetItem(QVariant(it->strokes[i]));
            lengthGrid->setItem( row, col++, item );

            if ((int)it->styles.size() > i) {
                item = createTableWidgetItem(QVariant(it->styles[i]));
                lengthGrid->setItem( row, col++, item );
            }
            row++;
        }
        set++;
    }

    lengthGrid->resizeColumnsToContents();
}

void SummaryImpl::fillSets( const std::vector<Set>& sets)
{
    // clearContents() does not reset selected line
    setGrid->setCurrentCell(0,0);

    setGrid->clearContents();

    std::vector<Set>::const_iterator i;

    setGrid->setRowCount(sets.size());

    int row=0;
    for (i=sets.begin(); i != sets.end(); ++i)
    {
        int col=0;
        QTableWidgetItem *item;

        item = createTableWidgetItem(QVariant(i->set));
        setGrid->setItem( row, col++, item );

        item = createTableWidgetItem(QVariant(i->duration.toString()));
        setGrid->setItem( row, col++, item );

        item = createTableWidgetItem(QVariant(i->lens));
        setGrid->setItem( row, col++, item );

        item = createTableWidgetItem(QVariant(i->dist));
        setGrid->setItem( row, col++, item );

        item = createTableWidgetItem(QVariant(i->strk));
        setGrid->setItem( row, col++, item );

        item = createTableWidgetItem(QVariant(i->speed));
        setGrid->setItem( row, col++, item );

        item = createTableWidgetItem(QVariant(i->effic));
        setGrid->setItem( row, col++, item );

        item = createTableWidgetItem(QVariant(i->rate));
        setGrid->setItem( row, col++, item );

        col++; // skip stroke

        item = createTableWidgetItem(QVariant(i->rest.toString("mm:ss")));
        setGrid->setItem( row, col++, item );

        QTime actual = getActualSwimTime(*i);
        QTime duration = i->duration;
        QString sign("");   // nothign for +
        if (actual > duration)
        {
            // we display as negative
            std::swap(actual, duration);
            sign = "-";
        }

        // now duration >= actual
        const int gapMS = actual.msecsTo(duration);

        // < 1sec: no display
        if (gapMS >= 1000)
        {
            const QTime gap = QTime(0, 0).addMSecs(gapMS);
            item = createTableWidgetItem(QVariant(sign + gap.toString()));
            setGrid->setItem( row, col++, item );
        }

        row++;
    }
    setGrid->resizeColumnsToContents();
}

//
// Select workout on date selected on calendar
//
void SummaryImpl::selectedDate( QDate d )
{
    int id = ds->findExercise(d);
    workoutGrid->selectRow(id);
}

//
// Workout in top grid selected.
//   populate set list
//   change view to workout view
//
void SummaryImpl::workoutSelected()
{
    // get selection
    // fill sets
    int row = workoutGrid->currentRow();
    QTableWidgetItem* it = workoutGrid->item(row,0);
    if (it->isSelected())
    {
        if (ds->Workouts().size())
        {
            const std::vector<Set>& sets = ds->Workouts()[row].sets;
            fillSets( sets );

            //    viewCombo->setCurrentIndex((int)WORKOUTS);
            calendarWidget->setSelectedDate( ds->Workouts()[row].date );
            fillLengths(ds->Workouts()[row]);
        }
        setData(ds->Workouts());

        graphWidget->update();
        volumeWidget->update();
        lengthWidget->update();
    }
    else
    {
        setGrid->clearContents();
        setGrid->setRowCount(0);

        lengthGrid->clearContents();
        lengthGrid->setRowCount(0);
    }
}

void SummaryImpl::syncButton()
{
    UploadImpl win(this);
    win.setDataStore(ds); // will populate our datastore
    win.exec();

    //Repopulate controls
    fillWorkouts(ds->Workouts());
}

/* setup path locations and colour preferences */
void SummaryImpl::configButton()
{
    ConfigImpl win(this);
    win.exec();
}

void SummaryImpl::editButton()
{
    int row = workoutGrid->currentRow();
    if (row >= 0)
    {
        const Workout & workout = ds->Workouts()[row];

        Edit edit(this, workout);
        if (edit.exec() == QDialog::Accepted)
        {
            Workout wrk;
            if ( edit.getModifiedWrk(wrk) )
            {
                ds->replaceWorkout(row, wrk);
                workoutSelected();
            }
        }
        else
        {
            if (edit.isDeleted())
            {
                ds->remove(row);
                //Repopulate controls
                fillWorkouts(ds->Workouts());
            }
        }
    }
}


//
// since we are a dialog application, handle escape as a close request
//
void SummaryImpl::keyPressEvent(QKeyEvent *event)
{
    if (event->key() != Qt::Key_Escape)
    {
        QDialog::keyPressEvent(event);
    }
    else
    {
        // close();
    }
}

void SummaryImpl::closeEvent(QCloseEvent *event)
{
    if (ds->hasChanged())
    {
        // TODO change to configurable datafile
        if (ds->getFile().isEmpty())
        {
            //user, username, home
            QString username = qgetenv("USER").constData();
            QString home = qgetenv("HOME").constData();

            QString filename = QString("%1/poolmate-%2.csv").arg(home).arg(username);

            //TODO add help/warning dialog
            QString file;
            file = QFileDialog::getSaveFileName(this,
                                                tr("Save Data"),
                                                filename,
                                                tr("Comma separated files (*.csv *.txt)"));

            if (!file.isEmpty())
            {
                //save settings
                QSettings settings("Swim","Poolmate");
                settings.setValue("dataFile", file);

                //update file
                ds->setFile(file);
                if (ds->save())
                    event->accept();
                else
                    event->ignore();
            }
            else
                event->ignore();
        }
        else
        {
            if (ds->save())
                event->accept();
            else
                event->ignore();
        }
    }
}

// TODO implement
void SummaryImpl::printButton()
{
    QPrinter printer(QPrinter::HighResolution);

    QPrintDialog *dialog = new QPrintDialog(&printer, this);
    dialog->setWindowTitle(tr("Print Chart"));

    //  if (editor->textCursor().hasSelection())
    //       dialog->addEnabledOption(QAbstractPrintDialog::PrintSelection);

    if (dialog->exec() != QDialog::Accepted)
        return;

    QPainter painter;
    painter.begin(&printer);

    double xscale = printer.pageRect().width()/double(graphWidget->width());
    double yscale = printer.pageRect().height()/double(graphWidget->height());
    double scale = qMin(xscale, yscale);

    // painter.translate(printer.paperRect().x() + printer.pageRect().width()/2,
    //               printer.paperRect().y() + printer.pageRect().height()/2);
    painter.translate(printer.paperRect().x(),
                      printer.paperRect().y());

    painter.scale(scale, scale);
    //    painter.translate(-width()/2, -height()/2);

    graphWidget->render(&painter);

    painter.translate(0, graphWidget->height()*2);
    volumeWidget->render(&painter);


    painter.end();

}

void SummaryImpl::onCheckClicked(bool)
{
    graphWidget->setGraphs(efficCheck->isChecked(),
                           strokeCheck->isChecked(),
                           rateCheck->isChecked(),
                           speedCheck->isChecked());
    graphWidget->update();

    lengthWidget->setGraphs(efficCheck->isChecked(),
                            strokeCheck->isChecked(),
                            rateCheck->isChecked(),
                            speedCheck->isChecked());
    lengthWidget->update();

}

void SummaryImpl::analysisButton()
{
    AnalysisImpl win(this);

    win.setDataStore(ds);
    win.exec();
}
