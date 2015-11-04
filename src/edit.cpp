/*
 * This file is part of PoolViewer
 * Copyright (c) 2015 Ivor Hewitt
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

#include <QMessageBox>

#include "edit.h"
#include "utilities.h"

namespace
{
void extractSetData(const Set & set, int & lengths, QTime & duration, double & average)
{
    lengths = set.times.size();
    duration = getActualSwimTime(set);

    const double actualTime = duration.msecsSinceStartOfDay() / 1000.0;

    if (lengths>0)
    {
        average = actualTime / lengths;
    }
    else
    {
        average = 0;
    }
}

QString formatSecondsToTimeWithMillis(const double value)
{
    QTime time = QTime(0, 0).addMSecs(value * 1000);
    QString result = time.toString("mm:ss.zzz");
    return result;
}

void fillSet(QTableWidget * grid, int row, const Set & set, const Workout & original)
{
    int lengths;
    QTime actual;
    double average;
    QTime duration;

    extractSetData(set, lengths, duration, average);
    const double actualTime = duration.msecsSinceStartOfDay() / 1000.0;

    if (average < 1) //increase to a sensible lower value
    {
        // Pick an average from workout data
        double real = original.totalduration.msecsSinceStartOfDay()
                - original.rest.msecsSinceStartOfDay();
        average = real / original.lengths / 1000;
    }

    actual = QTime(0,0).addMSecs(actualTime * 1000);

    const double recordedTime = set.duration.msecsSinceStartOfDay() / 1000;
    double gap = recordedTime - actualTime;

    int col=0;
    grid->setItem(row, col++, createTableWidgetItem(QVariant(row + 1)));
    grid->setItem(row, col++, createTableWidgetItem(QVariant(lengths)));

    grid->setItem(row, col++, createTableWidgetItem(QVariant(set.rest.toString())));

    grid->setItem(row, col++, createTableWidgetItem(QVariant(set.duration.toString())));
    grid->setItem(row, col++, createTableWidgetItem(QVariant(actual.toString())));

    if (gap >= 1)
    {
        grid->setItem(row, col++, createTableWidgetItem(QVariant(formatSecondsToTimeWithMillis(gap))));
    }
    else
    {
        grid->setItem(row, col++, createTableWidgetItem(QVariant("none")));
    }

    grid->setItem(row, col++, createTableWidgetItem(QVariant(formatSecondsToTimeWithMillis(average))));

    grid->setItem(row, col++, createTableWidgetItem(QVariant(set.strk)));
    grid->setItem(row, col++, createTableWidgetItem(QVariant(set.effic)));
    grid->setItem(row, col++, createTableWidgetItem(QVariant(set.speed)));

    //Highlight sets with significant time gaps.
    if ( gap >= average )
    {
        int i;
        for (i = 0; i < grid->columnCount(); ++i )
        {
            grid->item(row, i)->setBackgroundColor(QColor(Qt::lightGray));
        }
    }

    /*    if (modified)
    {
        int i;
        for (i = 0; i < setsGrid->columnCount(); ++i )
        {
            QFont defaultFont;
            defaultFont.setBold(true);
            setsGrid->item(currentSet, i)->setTextColor(QColor(Qt::red));
            setsGrid->item(currentSet, i)->setFont(defaultFont);
        }
    }*/
}
} //namespace

void Edit::populate(const Workout& _wrk)
{
    setsGrid->clearContents();
    setsGrid->setRowCount(_wrk.sets.size());
    setsGrid->setSelectionBehavior(QAbstractItemView::SelectRows);

    int i=0;
    for (i=0; i<_wrk.sets.size(); ++i)
    {
        fillSet(setsGrid, i, _wrk.sets[i], _wrk); //Fill sets
    }
}

Edit::Edit(QWidget *parent, const Workout & _wrk) :
    QDialog(parent), original(_wrk), deleted(false), changed(false)
{
    setupUi(this);

    modified = original;
    //populate
    populate(original);
}

bool Edit::getModifiedWrk(Workout &wrk) const
{
    if (deleted || !changed)
    {
        return false;
    }
    else
    {
        wrk = modified;
        return true;
    }
}

void Edit::on_newLengthsSpin_valueChanged(int /* extraLengths */)
{
    calculate();
}

/*
 * Increase the time steps on the gap?
 */
void Edit::on_gapTimeUsedSpin_valueChanged(double /* time */)
{
    calculate();
}

void Edit::on_strokesEdit_valueChanged(int /* strokes */)
{
    calculate();
}
void Edit::calculate()
{
    if (currentSet>=0)
    {
        setMod = setOrig;


        const int extraLengths = newLengthsSpin->value();

        if (extraLengths > 0) //Any updates?
        {
            int setLens;
            QTime duration;
            double setAvg;
            extractSetData(setOrig, setLens, duration, setAvg);


            const double gapTime = gapTimeUsedSpin->value();
            const double average = roundTo8thSecond(gapTime / extraLengths);
            lengthTimeEdit->setText(formatSecondsToTimeWithMillis(average));

            setMod.strk = strokesEdit->value();

            for (int i = 0; i < extraLengths; ++i)
            {
                // add a lengths keeping the rest syncronised
                ++setMod.lens;

                setMod.times.push_back(average);
                setMod.strokes.push_back(setMod.strk);
            }

            if (!setMod.styles.empty())
            {
                // add empty string at the end
                setMod.styles.resize(setMod.lens);
            }

            //Add pool length edit too
            setMod.dist = modified.pool * setMod.lens;

            //adjust duration
            setMod.duration = duration.addSecs( gapTime );
            int setsecs = setMod.duration.msecsSinceStartOfDay() / 1000;

            setMod.speed = 100 * setsecs / setMod.dist;
            setMod.effic = ((25 * setsecs / setMod.lens) + (25 * setMod.strk)) / modified.pool;
            setMod.rate = (60 * setMod.strk * setMod.lens) / setsecs;

            efficEdit->setText(QString("%1").arg(setMod.effic));
            speedEdit->setText(QString("%1").arg(setMod.speed));
        }
        else
        {
            lengthTimeEdit->clear();
        }
    }
}

/*
 * Delete entire workout
 **/
void Edit::on_deleteAllButton_clicked()
{
    int ret = QMessageBox::question(this, tr("Delete entire exercise"),
                                    tr("Delete this complete workout?"),
                                    QMessageBox::Yes|QMessageBox::No);
    // flag deleted
    if (ret == QMessageBox::Yes)
    {
        deleted = true;
        this->close();
    }
}

void Edit::on_deleteButton_clicked()
{
    int ret = QMessageBox::question(this, tr("Delete exercise sets"),
                                    tr("Delete the selected exercise sets?"),
                                    QMessageBox::Yes|QMessageBox::No);
    if (ret == QMessageBox::Yes)
    {
        std::vector<Set> sets;
        for (int i = 0; i < setsGrid->rowCount(); ++i)
        {
            QTableWidgetItem* it = setsGrid->item(i,0);
            if (!it->isSelected())
            {
                sets.push_back(modified.sets[i]);
            }
        }
        modified.sets = sets;
        changed=true;

        populate(modified);
        calculate();

        // recalc workout summary
    }
}

/*
 * Update the modified workout with modified set.
 */
void Edit::on_adjustButton_clicked()
{
    if (currentSet >= 0)
    {
        changed = true;
        modified.sets[currentSet] = setMod;
    }
    populate(modified);
}

//
// Populate edit fields on set selection
//
void Edit::on_setsGrid_currentCellChanged(int currentRow, int currentColumn, int previousRow, int previousColumn)
{
    // Fill edit fields with current set.
    currentSet = currentRow;

    if (currentSet >=0)
    {
        setOrig = modified.sets[currentRow];

        int lengths;
        QTime duration;
        double average;

        extractSetData(setOrig, lengths, duration, average);

        if (average < 1) //increase to a sensible lower value
        {
            // Pick an average from *original* workout data
            double real = original.totalduration.msecsSinceStartOfDay()
                    - original.rest.msecsSinceStartOfDay();
            average = real / original.lengths / 1000;
        }

        const double actualTime = duration.msecsSinceStartOfDay() / 1000.0;
        const double recordedTime = setOrig.duration.msecsSinceStartOfDay() / 1000;
        gap = recordedTime - actualTime;

        gapTimeUsedSpin->setMaximum(gap);
        gapTimeUsedSpin->setValue(gap);

        gapTimeEdit->setText(formatSecondsToTimeWithMillis(gap));

        const double equivalent = gap / average;
        lengthsEquivalentEdit->setText(QString("%1").arg(equivalent, 0, 'f', 2));

        newLengthsSpin->setValue(0);

        efficEdit->setText(QString("%1").arg(setMod.effic));
        speedEdit->setText(QString("%1").arg(setMod.speed));
        strokesEdit->setValue(setMod.strk);

/*        if (gap < 1)
        {
            adjustButton->setEnabled(false);
        }
        else
        {
            adjustButton->setEnabled(true);
        }*/
    }
}

void Edit::on_revertButton_clicked()
{
    changed=false;
    modified = original;
    populate(modified);
    calculate();
}

