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
            grid->item(row, i)->setBackground(QBrush(Qt::lightGray));
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
    lengthsGrid->clearContents();
    lengthsGrid->setRowCount(0);

    const int oldCurrentSet = currentSet;
    setsGrid->clearContents();
    setsGrid->setRowCount(_wrk.sets.size());

    size_t i=0;
    for (i=0; i<_wrk.sets.size(); ++i)
    {
        fillSet(setsGrid, i, _wrk.sets[i], _wrk); //Fill sets
    }

    // preserve original selection to avoid flickering
    setsGrid->setCurrentCell(oldCurrentSet, 1);
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
        synchroniseWorkout(wrk);
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
    if (currentSet >= 0)
    {
        setMod = setOrig;

        const int extraLengths = newLengthsSpin->value();

        if (extraLengths > 0) //Any updates?
        {
            int setLens;
            QTime duration;
            double setAvg;
            extractSetData(setMod, setLens, duration, setAvg);

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

            //adjust duration
            setMod.duration = duration.addSecs( gapTime );

            //Add pool length edit too
            synchroniseSet(setMod, modified);

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
        changed = true;

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
void Edit::on_setsGrid_currentCellChanged(int currentRow, int /*currentColumn*/, int /*previousRow*/, int /*previousColumn*/)
{
    lengthsGrid->clearContents();
    lengthsGrid->setRowCount(0);

    // Fill edit fields with current set.
    currentSet = currentRow;

    if (currentSet >=0)
    {
        setOrig = modified.sets[currentSet];
        setMod = setOrig;

        int lengths;
        QTime duration;
        double average;

        extractSetData(setMod, lengths, duration, average);

        if (average < 1) //increase to a sensible lower value
        {
            // Pick an average from *original* workout data
            double real = original.totalduration.msecsSinceStartOfDay()
                    - original.rest.msecsSinceStartOfDay();
            average = real / original.lengths / 1000;
        }

        const double actualTime = duration.msecsSinceStartOfDay() / 1000.0;
        const double recordedTime = setMod.duration.msecsSinceStartOfDay() / 1000;
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

        fillLengths(setMod);
    }
}

void Edit::fillLengths(const Set & set)
{
    lengthsGrid->setRowCount(set.times.size());
    for (uint i = 0; i < set.times.size(); ++i)
    {
        QTableWidgetItem *item;

        item = createTableWidgetItem(QVariant(set.times[i]));
        item->setData(Qt::UserRole, static_cast<uint>(i));
        lengthsGrid->setItem(i, 0, item);

        item = createTableWidgetItem(QVariant(set.strokes[i]));
        lengthsGrid->setItem(i, 1, item);
    }
    on_lengthsGrid_itemSelectionChanged();
}

void Edit::on_revertButton_clicked()
{
    changed = false;
    modified = original;
    populate(modified);
    calculate();
}

void Edit::on_squashLength_clicked()
{
    const int row = lengthsGrid->currentRow();
    if (row >= 0)
    {
        const QTableWidgetItem * item = lengthsGrid->item(row, 0);
        const uint pos = item->data(Qt::UserRole).toUInt();
        if (pos > 0)
        {
            setMod.times[pos - 1] += setMod.times[pos];
            setMod.strokes[pos - 1] += setMod.strokes[pos];

            --setMod.lens;
            setMod.times.erase(setMod.times.begin() + pos);
            setMod.strokes.erase(setMod.strokes.begin() + pos);

            if (pos < setMod.styles.size())
            {
                setMod.styles.erase(setMod.styles.begin() + pos);
            }

            synchroniseSet(setMod, modified);
            modified.sets[currentSet] = setMod;
            synchroniseWorkout(modified);
            changed = true;
            populate(modified);
        }
    }
}

void Edit::on_deleteLength_clicked()
{
    const int row = lengthsGrid->currentRow();
    if (row >= 0)
    {
        const QTableWidgetItem * item = lengthsGrid->item(row, 0);
        const uint pos = item->data(Qt::UserRole).toUInt();
        if (setMod.lens > 1)
        {
            setMod.duration = setMod.duration.addSecs(-setMod.times[pos]);
            setMod.rest = setMod.rest.addSecs(setMod.times[pos]);

            --setMod.lens;
            setMod.times.erase(setMod.times.begin() + pos);
            setMod.strokes.erase(setMod.strokes.begin() + pos);

            if (pos < setMod.styles.size())
            {
                setMod.styles.erase(setMod.styles.begin() + pos);
            }

            synchroniseSet(setMod, modified);
            modified.sets[currentSet] = setMod;
            synchroniseWorkout(modified);
            changed = true;
            populate(modified);
        }
    }
}

void Edit::on_balanceLength_clicked()
{
    const int row = lengthsGrid->currentRow();
    if (row >= 0)
    {
        const QTableWidgetItem * item = lengthsGrid->item(row, 0);
        const uint pos = item->data(Qt::UserRole).toUInt();
        if (pos > 0)
        {
            const double totalTime = setMod.times[pos] + setMod.times[pos - 1];
            setMod.times[pos - 1] = roundTo8thSecond(totalTime * 0.5);
            setMod.times[pos] = totalTime - setMod.times[pos - 1];

            const int totalStrokes = setMod.strokes[pos] + setMod.strokes[pos - 1];
            setMod.strokes[pos - 1] = totalStrokes / 2;
            setMod.strokes[pos] = totalStrokes - setMod.strokes[pos - 1];

            // no need to synchronise as totals are preserved
            modified.sets[currentSet] = setMod;
            changed = true;
            populate(modified);
        }
    }
}

void Edit::on_lengthsGrid_itemSelectionChanged()
{
    const int row = lengthsGrid->currentRow();
    if (row >= 0)
    {
        const QTableWidgetItem * item = lengthsGrid->item(row, 0);
        const uint pos = item->data(Qt::UserRole).toUInt();

        squashLength->setEnabled(pos > 0);
        balanceLength->setEnabled(pos > 0);
        deleteLength->setEnabled(setMod.lens > 1);

        // give a visual indication of which lengths are modified
        squashLength->setText(QString("Squash %1 -> %2").arg(1 + pos).arg(pos));
        balanceLength->setText(QString("Balance %1 && %2").arg(1 + pos).arg(pos));
        deleteLength->setText(QString("Delete %1").arg(1 + pos));
    }
    else
    {
        squashLength->setEnabled(false);
        balanceLength->setEnabled(false);
        deleteLength->setEnabled(false);
    }
}
