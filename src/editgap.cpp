#include "editgap.h"
#include "utilities.h"

namespace
{
    void extractSetData(const Set & set, int & lengths, QTime & duration, double & average)
    {
        lengths = set.times.size();
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

void EditGap::setOriginalSet(const Set * _set)
{
    original = _set;

    int lengths;
    QTime duration;
    double average;
    extractSetData(*original, lengths, duration, average);

    const double actualTime = duration.msecsSinceStartOfDay() / 1000.0;
    const double recordedTime = original->duration.msecsSinceStartOfDay() / 1000;
    gap = recordedTime - actualTime;

    // these never change
    displaySet(setGrid, 0, *original);
    gapTimeEdit->setText(formatSecondsToTimeWithMillis(gap));

    const double equivalent = gap / average;
    lengthsEquivalentEdit->setText(QString("%1").arg(equivalent, 0, 'f', 2));

    // simulate the 0 to update GUI properly
    on_lengthsSpin_valueChanged(0);
}

const Set & EditGap::getModifiedSet() const
{
    return modified;
}

void EditGap::update()
{
    // only update modified row on grid
    displaySet(setGrid, 1, modified);
}

void EditGap::on_lengthsSpin_valueChanged(int extraLengths)
{
    modified = *original;

    if (extraLengths > 0)
    {
        const double average = roundTo8thSecond(gap / extraLengths);
        averageTimeEdit->setText(formatSecondsToTimeWithMillis(average));

        for (int i = 0; i < extraLengths; ++i)
        {
            // add a lengths keeping the rest syncronised
            ++modified.lens;

            modified.times.push_back(average);
            modified.strokes.push_back(modified.strk);
        }

        if (!modified.styles.empty())
        {
            // add empty string at the end
            modified.styles.resize(modified.lens);
        }

        modified.dist = modified.dist * modified.lens / original->lens; // this should be exact integer arithmetic
        modified.effic = modified.effic * original->lens / modified.lens;
        modified.speed = modified.speed * original->lens / modified.lens;
    }
    else
    {
        averageTimeEdit->clear();
    }
    update();
}
