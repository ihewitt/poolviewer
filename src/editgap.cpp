#include "editgap.h"
#include "utilities.h"

namespace
{
    void extractSetData(const Set & set, int pool, int & lengths, QTime & duration, double & average, double &speed)
    {
        lengths = set.times.size();
        duration = getActualSwimTime(set);

        const double actualTime = duration.msecsSinceStartOfDay() / 1000.0;
        average = actualTime / lengths;

        const double distance = lengths * pool;
        const double recordedTime = set.duration.msecsSinceStartOfDay() / 1000.0;
        speed = recordedTime / distance * 100.0;
    }

    void displaySet(QTableWidget * grid, int row, const Set & set, int pool)
    {
        int lengths;
        QTime actual;
        double average;
        double speed;
        extractSetData(set, pool, lengths, actual, average, speed);

        QTableWidgetItem * lengthsItem = createTableWidgetItem(QVariant(lengths));
        grid->setItem(row, 0, lengthsItem);

        QTableWidgetItem * displayedItem = createTableWidgetItem(QVariant(set.duration.toString()));
        grid->setItem(row, 1, displayedItem);

        QTableWidgetItem * actualItem = createTableWidgetItem(QVariant(actual.toString()));
        grid->setItem(row, 2, actualItem);

        QTableWidgetItem * averageItem = createTableWidgetItem(QVariant(average));
        grid->setItem(row, 3, averageItem);

        QTableWidgetItem * speedItem = createTableWidgetItem(QVariant(speed));
        grid->setItem(row, 4, speedItem);
    }
}

EditGap::EditGap(QWidget *parent) :
    QDialog(parent)
{
    setupUi(this);
}

void EditGap::setOriginalSet(const Set * _set, int _pool)
{
    original = _set;
    pool = _pool;
    modified = *original;

    int lengths;
    QTime duration;
    double speed;
    double average;
    extractSetData(*original, pool, lengths, duration, average, speed);

    const double actual = duration.msecsSinceStartOfDay() / 1000.0;
    const double recorded = original->duration.msecsSinceStartOfDay() / 1000;
    gap = recorded - actual;

    // these never change
    displaySet(setGrid, 0, *original, pool);
    gapTimeLCD->display(gap);

    const double equivalent = gap / average;
    equivalentLCD->display(equivalent);

    update();
}

void EditGap::update()
{
    displaySet(setGrid, 1, modified, pool);
}


void EditGap::on_lengthsSpin_valueChanged(int arg1)
{
    modified = *original;

    if (arg1 > 0)
    {
        const double average = gap / arg1;

        for (size_t i = 0; i < arg1; ++i)
        {
            ++modified.lens;
            modified.times.push_back(average);
            modified.strokes.push_back(modified.strk);
        }
    }
    update();
}
