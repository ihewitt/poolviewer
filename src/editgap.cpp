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

#include "editgap.h"
#include "utilities.h"

namespace
{
    template<typename T>
    void appendToVectorIsCorrectSize(std::vector<T> & v, size_t size, const T & value)
    {
        if (v.size() == size)
        {
            v.push_back(value);
        }
    }

    void extractSetData(const Set & set, int & lengths, QTime & duration, double & average)
    {
        lengths = set.lens;
        duration = getActualSwimTime(set);

        const double actualTime = duration.msecsSinceStartOfDay() / 1000.0;
        average = actualTime / lengths;
    }

    QString formatSecondsToTimeWithMillis(const double value)
    {
        QTime time = QTime(0, 0).addMSecs(value * 1000);
        QString result = time.toString("mm:ss.zzz");
        return result;
    }

    void displaySet(QTableWidget * grid, int row, const Set & set)
    {
        int lengths;
        QTime actual;
        double average;
        extractSetData(set, lengths, actual, average);

        QTableWidgetItem * lengthsItem = createTableWidgetItem(QVariant(lengths));
        grid->setItem(row, 0, lengthsItem);

        QTableWidgetItem * displayedItem = createTableWidgetItem(QVariant(set.duration.toString()));
        grid->setItem(row, 1, displayedItem);

        QTableWidgetItem * actualItem = createTableWidgetItem(QVariant(actual.toString()));
        grid->setItem(row, 2, actualItem);

        QTableWidgetItem * averageItem = createTableWidgetItem(QVariant(formatSecondsToTimeWithMillis(average)));
        grid->setItem(row, 3, averageItem);

        QTableWidgetItem * speedItem = createTableWidgetItem(QVariant(set.speed));
        grid->setItem(row, 4, speedItem);
    }

}

EditGap::EditGap(QWidget *parent) :
    QDialog(parent)
{
    setupUi(this);
}

bool EditGap::setOriginalSet(const Set * _set, int _pool)
{
    original = _set;
    pool = _pool;

    // validation
    if (original->times.size() != original->lens)
    {
        // no times data
        return false;
    }

    // validation
    if (!original->lens)
    {
        // this is not strictly required, but it would require entering stroke data
        // and to recompute the speed and efficiency for the set from the lengths
        return false;
    }

    int lengths;
    QTime duration;
    double average;
    extractSetData(*original, lengths, duration, average);

    const double actualTime = duration.msecsSinceStartOfDay() / 1000.0;
    const double recordedTime = original->duration.msecsSinceStartOfDay() / 1000;
    gap = recordedTime - actualTime;

    if (gap < 1)
    {
        // no point in doing anything
        return false;
    }

    gapTimeUsedSpin->setMaximum(gap);
    gapTimeUsedSpin->setValue(gap);

    // these never change
    displaySet(setGrid, 0, *original);
    gapTimeEdit->setText(formatSecondsToTimeWithMillis(gap));

    const double equivalent = gap / average;
    lengthsEquivalentEdit->setText(QString("%1").arg(equivalent, 0, 'f', 2));

    // simulate the 0 to update GUI properly
    calculate();

    return true;
}

const Set & EditGap::getModifiedSet() const
{
    return modified;
}

void EditGap::on_newLengthsSpin_valueChanged(int /* extraLengths */)
{
    calculate();
}

void EditGap::on_gapTimeUsedSpin_valueChanged(double /* time */)
{
    calculate();
}

void EditGap::calculate()
{
    modified = *original;

    const int extraLengths = newLengthsSpin->value();

    if (extraLengths > 0)
    {
        const double gapTime = gapTimeUsedSpin->value();
        const double average = roundTo8thSecond(gapTime / extraLengths);
        lengthTimeEdit->setText(formatSecondsToTimeWithMillis(average));

        for (int i = 0; i < extraLengths; ++i)
        {
            // add a lengths keeping the rest syncronised

            // only append if vector have already correct size
            appendToVectorIsCorrectSize(modified.times, modified.lens, average);
            appendToVectorIsCorrectSize(modified.strokes, modified.lens, modified.strk);
            appendToVectorIsCorrectSize(modified.styles, modified.lens, QString());

            // and finally update distance
            ++modified.lens;
            modified.dist += pool;
        }

        // this will break down if the original set did not have any lengths
        // they should be recomputed from the lengths data
        modified.effic = modified.effic * original->lens / modified.lens;
        modified.speed = modified.speed * original->lens / modified.lens;
    }
    else
    {
        lengthTimeEdit->clear();
    }
    // only update modified row on grid
    displaySet(setGrid, 1, modified);
}
